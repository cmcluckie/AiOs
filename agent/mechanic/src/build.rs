// SPDX-License-Identifier: GPL-2.0-only
//! A builder: one fresh session, one job, then gone (findings 0027, SPEC.agent.4).
//! It is handed a red test, looks at its own copy of the repo at the job's commit,
//! hands in a fix, and the judge runs it on the jig.  The loop is the runtime's
//! (CivicCanada's, ported); the tools, the limits and the judging are jig/fix's, so
//! the two benches differ only in what is being raced.

use crate::judge::{self, Jig};
use crate::look;
use crate::note;
use crate::record::{self, Runs};
use crate::util::plain;
use runtime::model::{Keys, Model, Stop};
use runtime::react::{self, Ended, Hooks, Step};
use runtime::{prompts, room};
use serde_json::{json, Value};
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

pub const MAX_FILES: usize = 32;

#[derive(Debug)]
pub struct Job {
    pub job: String,
    pub test: String,
    pub exam: String, // an exam item to lift, in place of a test to fix (findings 0030)
    pub at: String,
    pub try_: u64,
    pub prompt: String,
    pub model: String,
    pub num_ctx: u64,
    pub suite: String,
    pub hint: String,
    pub item: String,
    pub trial: String,
    pub max_turns: u64,
    pub max_tokens: u64,
    pub think_tokens: u64,
    pub look_chars: u64,
    pub room_share: u64,
    pub gear: u8,
    pub max_usd: Option<f64>,
    pub deadline: Option<u64>,
}

fn read_job(path: &str) -> Result<Job, String> {
    let text = fs::read_to_string(path).map_err(|_| format!("{path} could not be read"))?;
    let t: toml::Table = toml::from_str(&text).map_err(|e| crate::plan::toml_error(path, &text, &e))?;
    let known = [
        "job", "test", "at", "try", "prompt", "model", "num_ctx", "suite", "hint", "item", "max_turns", "max_tokens", "think_tokens",
        "look_chars", "room_share", "gear", "max_usd", "deadline", "trial", "exam",
    ];
    if let Some(k) = t.keys().find(|k| !known.contains(&k.as_str())) {
        return Err(format!("{k} is not something a job has"));
    }
    let s = |k: &str, need: bool| -> Result<String, String> {
        match t.get(k) {
            Some(toml::Value::String(v)) => Ok(v.clone()),
            Some(_) => Err(format!("{k} is a string")),
            None if need => Err(format!("no {k}")),
            None => Ok(String::new()),
        }
    };
    let n = |k: &str, d: Option<u64>, lo: u64, hi: u64| -> Result<u64, String> {
        let v = match t.get(k) {
            Some(toml::Value::Integer(v)) => *v,
            Some(_) => return Err(format!("{k} is a whole number")),
            None => return d.ok_or(format!("no {k}")),
        };
        if v < lo as i64 || v > hi as i64 {
            return Err(format!("{k} is {v}, outside {lo} to {hi}"));
        }
        Ok(v as u64)
    };
    let max_usd = match t.get("max_usd") {
        None => None,
        Some(toml::Value::Float(f)) if *f >= 0.0 => Some(*f),
        Some(toml::Value::Integer(i)) if *i >= 0 => Some(*i as f64),
        Some(_) => return Err("max_usd is dollars, 0 or more".into()),
    };
    let exam = s("exam", false)?;
    let mut j = Job {
        job: s("job", true)?,
        test: s("test", exam.is_empty())?,
        exam,
        at: s("at", true)?,
        try_: n("try", Some(1), 1, 1000)?,
        prompt: s("prompt", true)?,
        model: s("model", true)?,
        num_ctx: n("num_ctx", None, 2048, 1_048_576)?,
        suite: s("suite", false)?,
        hint: s("hint", false)?,
        item: s("item", false)?,
        trial: s("trial", false)?,
        max_turns: n("max_turns", Some(60), 1, 500)?,
        // what a reply may say and call, and apart from it, what it may think first:
        // two builders on waits died thinking past one shared limit (findings 0029)
        max_tokens: n("max_tokens", Some(12000), 256, 200_000)?,
        think_tokens: n("think_tokens", Some(8000), 0, 200_000)?,
        look_chars: n("look_chars", Some(8000), 500, 100_000)?,
        room_share: n("room_share", Some(60), 10, 90)?,
        gear: n("gear", Some(1), 1, 2)? as u8,
        max_usd,
        deadline: match t.get("deadline") {
            None => None,
            Some(toml::Value::Integer(v)) if *v > 0 => Some(*v as u64),
            Some(_) => return Err("deadline is a time in seconds since 1970".into()),
        },
    };
    let hex = !j.at.is_empty() && j.at.len() >= 7 && j.at.chars().all(|c| c.is_ascii_hexdigit() && !c.is_ascii_uppercase());
    if !plain(&j.job, "._-@") || !plain(&j.test, "._-@") || !plain(&j.suite, " ._-@") || !hex
        || !plain(&j.model, "._-:/") || !prompts::valid(&j.prompt)
        || !j.item.chars().all(|c| c.is_ascii_lowercase() || c.is_ascii_digit() || c == '-' || c == '_')
        || !plain(&j.trial, "-_")
        || !j.exam.chars().all(|c| c.is_ascii_lowercase() || c.is_ascii_digit() || c == '-' || c == '_')
        || (!j.exam.is_empty() && !j.test.is_empty())
    {
        return Err("job, test, at, model, prompt, suite or item has a character a job may not".into());
    }
    if j.item.is_empty() && !j.exam.is_empty() {
        // jig/sit records a sitting of an item under the item's name, so the two line up
        j.item = j.exam.clone();
    }
    if j.item.is_empty() {
        // the old bench's name for a fix of this test, so the two line up (jig/fix)
        let t = j.test.split('@').next().unwrap_or("");
        j.item = format!("fix-{}", t.replace('.', "-")).to_lowercase();
    }
    Ok(j)
}

/// key = "value" on its own line, as the repo writes tests and specs
fn field(path: &Path, key: &str) -> Option<String> {
    let text = fs::read_to_string(path).ok()?;
    let t: toml::Table = toml::from_str(&text).ok()?;
    t.get(key)?.as_str().map(String::from)
}

pub(crate) fn say(quiet: bool, s: &str) {
    if !quiet {
        eprintln!("  {s}");
    }
}

pub(crate) fn now() -> u64 {
    SystemTime::now().duration_since(UNIX_EPOCH).map(|d| d.as_secs()).unwrap_or(0)
}

const TOOLS: &str = r#"[{"type":"function","function":{"name":"look","description":"Look at the repo, as it stands with the test red. command is list (args: a folder), read (args: a file, then optionally the first and last line to show, or give them as start_line and end_line; at most 400 lines at a time; it answers with the file, the lines shown and how many it has, then each line with its number) or find (args: a word, then optionally a folder or file to look in; otherwise pool, specs, tests, index, findings and jig).","parameters":{"type":"object","properties":{"why":{"type":"string","description":"What you expect to learn from this look, in one sentence. A look without it is refused."},"command":{"type":"string","enum":["list","read","find"]},"args":{"type":"array","items":{"type":"string"}},"start_line":{"type":"integer"},"end_line":{"type":"integer"}},"required":["why","command","args"]}}},{"type":"function","function":{"name":"edit_file","description":"Change a file on disk, in your copy of the repo: old, copied exactly from the file and there exactly once, is replaced by new. Only existing files under pool/, all in the same part. Nothing is judged until you hand in; what you look at afterwards shows the change.","parameters":{"type":"object","properties":{"path":{"type":"string"},"old":{"type":"string","description":"The lines to replace, exactly as they are in the file, enough of them to be there once."},"new":{"type":"string"}},"required":["path","old","new"]}}},{"type":"function","function":{"name":"submit_fix","description":"Hand in what you changed on disk; files, if given, are first written whole: each its path (an existing file under pool/, all in the same part, e.g. pool/PART.gate@2/) and its whole new content. The fix is judged by every test: a pass ends the run; otherwise you are told what the tests printed, your changes stay on disk, and you may change them and hand in again while you have calls left. A run that ends without a hand-in has what is on disk handed in for it.","parameters":{"type":"object","properties":{"files":{"type":"array","items":{"type":"object","properties":{"path":{"type":"string"},"content":{"type":"string"}},"required":["path","content"]}}}}}}]"#;

/// What the builder's tools and limits do between the runtime's calls.
struct Builder<'a> {
    j: &'a Job,
    dir: PathBuf,
    base: PathBuf,     // the repo at the job's commit, never changed
    work: PathBuf,     // the builder's copy of it, which its edits change
    touched: Vec<String>, // files it has changed on disk
    judged: String,    // what was on disk at the last hand-in
    by_program: bool,  // the last hand-in was the program's, at the end
    code: String,
    budget: u64,            // the conversation's share of the card, system prompt and tools included
    est: room::Estimate,    // characters over four, calibrated by what the model counts
    system: String,
    jig: &'a Jig,
    runs: &'a Runs,
    recorded: String,
    quiet: bool,
    hand: u64,
    paths: Vec<String>,
    result: String,
    line: String,
    lines: u64,
    folded: usize,
    jig_down: bool,
}

impl Builder<'_> {
    /// paths the builder may change: checked with the ones it has changed already,
    /// so all of them stay existing files of one part under pool/
    fn may_change(&self, more: &[String]) -> Result<(), String> {
        let mut all: Vec<String> = self.touched.clone();
        for p in more {
            if !all.contains(p) {
                all.push(p.clone());
            }
        }
        judge::check(&self.base, &all)
    }

    /// one exact edit on disk, in the builder's working copy: old must be there once
    fn edit(&mut self, args: &Value) -> String {
        let (Some(p), Some(old), Some(new)) = (args["path"].as_str(), args["old"].as_str(), args["new"].as_str()) else {
            return "edit_file needs path, old and new.  Nothing was changed.".into();
        };
        if let Err(why) = self.may_change(&[p.to_string()]) {
            return format!("refused: {why}.  Nothing was changed.");
        }
        let file = self.work.join(p);
        let text = fs::read_to_string(&file).unwrap_or_default();
        match text.matches(old).count() {
            0 => return format!("{p} does not hold old exactly as given.  Nothing was changed; look again and copy it exactly."),
            1 => {}
            n => return format!("{p} holds old {n} times; give more of the lines around it so it is there once.  Nothing was changed."),
        }
        if old.is_empty() || fs::write(&file, text.replacen(old, new, 1)).is_err() {
            return format!("{p} could not be changed.  Nothing was changed.");
        }
        self.note_touched(p);
        say(self.quiet, &format!("edit {p}"));
        format!("Edited {p} on disk.  Nothing is judged until you hand it in with submit_fix; a run that ends without a hand-in has what is on disk handed in for it.")
    }

    fn note_touched(&mut self, p: &str) {
        if !self.touched.iter().any(|t| t == p) {
            self.touched.push(p.to_string());
        }
    }

    /// the files a hand-in gives whole are written to disk first, checked before anything is
    fn write_files(&mut self, args: &Value) -> Result<(), String> {
        let mut files = Vec::new();
        for f in args["files"].as_array().into_iter().flatten() {
            match (f["path"].as_str(), f["content"].as_str()) {
                (Some(p), Some(c)) if files.len() < MAX_FILES => files.push((p.to_string(), c.to_string())),
                _ => return Err(format!("each file is a path and its whole content, at most {MAX_FILES}")),
            }
        }
        let paths: Vec<String> = files.iter().map(|(p, _)| p.clone()).collect();
        self.may_change(&paths)?;
        for (p, c) in &files {
            fs::write(self.work.join(p), c).map_err(|_| format!("could not write {p}"))?;
            self.note_touched(p);
        }
        Ok(())
    }

    /// what is on disk now: each changed file and its content, for knowing whether
    /// it was judged already
    fn on_disk(&self) -> String {
        self.touched.iter().map(|p| format!("{p}\n{}", fs::read_to_string(self.work.join(p)).unwrap_or_default())).collect()
    }

    /// the files changed on disk, run against the suite on the jig.  by: the
    /// builder, or the program when the builder ended without handing them in
    fn judge_disk(&mut self, by_program: bool) -> String {
        self.hand += 1;
        let h = self.hand;
        let fixed = self.dir.join("fixed");
        let _ = fs::remove_dir_all(&fixed);
        let mut diff = String::new();
        let mut changed = Vec::new();
        for p in self.touched.clone() {
            let to = fixed.join(&p);
            if let Some(parent) = to.parent() {
                let _ = fs::create_dir_all(parent);
            }
            let _ = fs::copy(self.work.join(&p), &to);
            let out = Command::new("diff").arg(self.base.join(&p)).arg(&to).output();
            let d = out.map(|o| String::from_utf8_lossy(&o.stdout).to_string()).unwrap_or_default();
            if !d.is_empty() {
                changed.push(p.clone());
            }
            diff += &format!("=== {p}\n{d}");
        }
        self.lines = diff.lines().filter(|l| l.starts_with('<') || l.starts_with('>')).count() as u64;
        let _ = fs::write(self.dir.join("output.txt"), &diff);
        self.paths = changed;
        self.judged = self.on_disk();
        self.by_program = by_program;
        let who = if by_program { "handed in by the program, from what was on disk" } else { "" };
        say(self.quiet, &format!("hand-in {h}{}{}, {} file(s) changed; running the suite on the jig", if by_program { ", " } else { "" }, who, self.paths.len()));
        let over = if fixed.exists() { Some(fixed.as_path()) } else { None };
        let after = match self.jig.run(&self.j.at, &self.j.suite, over) {
            Ok(a) => a,
            Err(why) => {
                // the jig, not the fix: the run stops, and the note is not written from an error
                self.jig_down = true;
                self.result = "fail".into();
                self.line = why.clone();
                let _ = fs::write(self.dir.join("suite-did-not-run.txt"), &why);
                return format!("Hand-in {h} was not judged: {why}.");
            }
        };
        let _ = fs::write(self.dir.join("suite.txt"), &after);
        let before = fs::read_to_string(self.dir.join("before.txt")).unwrap_or_default();
        let v = judge::verdict(&before, &after, &self.code);
        let _ = fs::write(self.dir.join("newly-red.txt"), v.newly_red.join("\n"));
        self.result = if v.pass { "pass" } else { "fail" }.into();
        self.line = if self.paths.is_empty() && by_program {
            format!("nothing was changed on disk, so the tests ran on the commit as it was: {}", v.line)
        } else {
            v.line.clone()
        };
        say(self.quiet, &format!("hand-in {h}: {}: {}", self.result, runtime::model::cut(&self.line, 140)));
        if v.pass {
            return format!("Hand-in {h} judged: pass.  {}.", v.line);
        }
        let mut a = format!("Hand-in {h} judged: fail.  {}.", v.line);
        if !v.newly_red.is_empty() {
            a += &format!("\nWhat the tests that went red printed:\n{}", runtime::model::cut(&v.newly_red.join("\n"), 2000));
        }
        a + "\nWhat you changed is still on disk.  Change it, with edit_file or whole files, and hand it in again with submit_fix."
    }

    fn submit(&mut self, args: &Value) -> String {
        if let Err(why) = self.write_files(args) {
            say(self.quiet, &format!("hand-in refused: {why}"));
            return format!("Hand-in refused: {why}.  Nothing was written or run.");
        }
        if self.touched.is_empty() {
            return "Nothing is changed on disk yet: change a file with edit_file, or give whole files in files, then hand in.".into();
        }
        self.judge_disk(false)
    }
}

impl Hooks for Builder<'_> {
    fn tools(&self) -> Vec<Value> {
        serde_json::from_str(TOOLS).expect("the builder's tools are JSON")
    }

    fn before(&mut self, turn: u32, messages: &mut Vec<Value>, tin: u64, tout: u64) -> Result<(), (String, String)> {
        if self.jig_down {
            return Err(("error".into(), format!("the suite did not run on the jig on hand-in {}", self.hand)));
        }
        if let Some(d) = self.j.deadline {
            if now() >= d {
                return Err(("time".into(), format!("before call {turn}: the part's minutes are used")));
            }
        }
        // what is left for the messages once the system prompt and the tools are sent
        let fixed = self.est.tokens(&self.system) + self.est.tokens(TOOLS);
        let dropped = room::drop_old_measured(messages, self.budget.saturating_sub(fixed), &self.est);
        if dropped > 0 {
            self.folded += dropped;
            say(self.quiet, &format!("call {turn}: {dropped} characters of old answers dropped to make room"));
        }
        if self.j.gear == 2 {
            // a paid call is asked first of the job's share, then of the month's cap,
            // with what this run has spent and the most the next call may cost
            let spent = self.runs.cost(&self.recorded, tin, tout).unwrap_or(0.0);
            let next = max_call();
            if let Some(m) = self.j.max_usd {
                if spent + next > m {
                    return Err(("budget".into(), format!("before call {turn}: the job's share of the part's budget, ${m:.2}, has no room for another call after ${spent:.4}")));
                }
            }
            if let Err(why) = self.runs.room(spent + next) {
                return Err(("budget".into(), format!("before call {turn}: {why}")));
            }
        }
        Ok(())
    }

    fn call(&mut self, name: &str, args: &Value) -> Step {
        match name {
            "look" => {
                let why = args["why"].as_str().unwrap_or("").trim();
                // reason, then act: a look says why, or it is refused and not run (decision 0092)
                if why.is_empty() {
                    return Step::Answer("refused: say why you are looking, in why: what you expect to learn from this look, in one sentence.  Nothing was looked at.".into());
                }
                let cmd = args["command"].as_str().unwrap_or("");
                let mut a: Vec<String> = args["args"].as_array().into_iter().flatten().filter_map(|v| v.as_str()).filter(|s| !s.is_empty()).take(6).map(String::from).collect();
                if let Some(s) = args["start_line"].as_u64() {
                    a.push(s.to_string());
                    if let Some(e) = args["end_line"].as_u64() {
                        a.push(e.to_string());
                    }
                }
                say(self.quiet, &format!("look {cmd} {}; why: {}", a.first().map(String::as_str).unwrap_or(""), runtime::model::cut(why, 120)));
                let (answer, _) = look::look(&self.work, cmd, &a);
                // cut, and said so, so one read cannot fill the card (CivicCanada's Truncate)
                Step::Answer(room::cap(&answer, self.j.look_chars as usize))
            }
            "edit_file" => Step::Answer(self.edit(args)),
            "submit_fix" => {
                let a = self.submit(args);
                if self.result == "pass" {
                    Step::Finish(a, "pass".into(), String::new())
                } else {
                    Step::Answer(a)
                }
            }
            other => Step::Answer(format!("There is no tool {other}.  The tools are look, edit_file and submit_fix.")),
        }
    }

    fn after(&mut self, turn: u32, left: u32) -> Option<String> {
        let mut n = format!("Call {turn} of {} done; {left} left.", self.j.max_turns);
        if left <= 10 {
            n += "  Ten or fewer left: hand in your fix with submit_fix now.  A run that ends without one has only what is on disk handed in for it.";
        }
        Some(n)
    }

    fn sent(&mut self, counted: u64, messages: &[Value]) {
        // the model's own count of what was sent sets the estimate from then on
        let plain = room::tokens(&self.system) + room::tokens(TOOLS) + room::size(messages);
        self.est.calibrate(counted, plain);
    }

    fn nudge(&mut self, stop: Stop, turn: u32, left: u32) -> String {
        let what = if stop == Stop::MaxTokens {
            "Your reply was cut off at its limit, before you handed in a fix.  Keep the next reply shorter.".to_string()
        } else if self.hand > 0 {
            "You stopped without handing in a corrected fix.".to_string()
        } else {
            "You stopped without handing in a fix.".to_string()
        };
        format!("{what}  Call {turn} of {} done; {left} left.  Look further if you need to, then hand it in with submit_fix.", self.j.max_turns)
    }
}

pub fn max_call() -> f64 {
    std::env::var("AGENT_MAX_COST").ok().and_then(|v| v.parse::<f64>().ok()).filter(|v| *v > 0.0).unwrap_or(0.50)
}

pub(crate) fn absolute(p: &str) -> PathBuf {
    let p = PathBuf::from(p);
    if p.is_absolute() { p } else { std::env::current_dir().unwrap_or_default().join(p) }
}

/// Runs the job; 0 a pass, 1 not a pass, 2 cannot run here.
pub fn run(jobpath: &str) -> i32 {
    let start = Instant::now();
    let quiet = std::env::var_os("AGENT_QUIET").is_some();
    let j = match read_job(jobpath) {
        Ok(j) => j,
        Err(why) => {
            eprintln!("agent: job {jobpath}: {why}");
            return 2;
        }
    };
    if !j.exam.is_empty() {
        return crate::lift::run(&j, start, quiet);
    }
    let repo = absolute(&std::env::var("AGENT_REPO").ok().filter(|v| !v.is_empty()).unwrap_or_else(|| ".".into()));
    // made whole from here, since the judge's command changes folder
    let out = absolute(&std::env::var("AGENT_OUT").ok().filter(|v| !v.is_empty()).unwrap_or_else(|| "out/agent".into()));
    let dir = out.join(format!("{}-try{}", j.job, j.try_));
    let base = dir.join("base");
    let (system, print) = match prompts::load(&repo.join("prompts"), &j.prompt) {
        Ok(p) => p,
        Err(why) => {
            eprintln!("agent: {why}");
            return 2;
        }
    };
    let hint = if j.hint.is_empty() {
        None
    } else {
        match fs::read_to_string(&j.hint) {
            Ok(h) => Some(h),
            Err(_) => {
                eprintln!("agent: no hint at {}", j.hint);
                return 2;
            }
        }
    };
    let recorded = format!("{}:{}", if j.gear == 2 { "bedrock" } else { "ollama" }, j.model);
    let runs = Runs::new(&repo);
    let keys: Option<Keys> = if j.gear == 2 {
        if runs.cost(&recorded, 0, 0).is_none() {
            eprintln!("agent: no price for {recorded}: a model whose calls cannot be costed is not called");
            return 2;
        }
        match record::keys() {
            Ok(k) => Some(k),
            Err(why) => {
                eprintln!("agent: Bedrock: {why}");
                return 2;
            }
        }
    } else {
        None
    };
    // a fresh copy of the repo at the job's commit: what the builder sees, and nothing else
    let _ = fs::remove_dir_all(&dir);
    let work = dir.join("work");
    let _ = fs::create_dir_all(&base);
    // the copy twice: base never changes, work is where the builder's edits land
    let copied = Command::new("sh")
        .arg("-c")
        .arg("git -C \"$1\" archive \"$2\" | tar -xf - -C \"$3\" && rm -rf \"$3/runs\" \"$3/out\" && cp -a \"$3\" \"$4\"")
        .args(["copy", &repo.to_string_lossy(), &j.at, &base.to_string_lossy(), &work.to_string_lossy()])
        .status()
        .map(|s| s.success())
        .unwrap_or(false);
    if !copied {
        eprintln!("agent: could not copy the repo at {}", j.at);
        return 2;
    }
    let test_file = base.join(format!("tests/{}.toml", j.test));
    let (Some(code), Some(binds)) = (field(&test_file, "code"), field(&test_file, "binds")) else {
        eprintln!("agent: no test {} at {}", j.test, j.at);
        return 2;
    };
    let clause = field(&base.join(format!("specs/{}.toml", binds.trim_start_matches("SPEC."))), "clause")
        .unwrap_or_else(|| format!("(no clause found for {binds})"));

    // before spending anything: the suite on the jig as it was, and the test must be red
    let jig = Jig::from_env();
    say(quiet, &format!("running the suite on the jig at {}, to see what is red before", j.at));
    let before = match jig.run(&j.at, &j.suite, None) {
        Ok(b) => b,
        Err(why) => {
            eprintln!("agent: {why}");
            return 2;
        }
    };
    let _ = fs::write(dir.join("before.txt"), &before);
    let Some(redline) = before.lines().find(|l| l.strip_prefix("FAIL  ").map(|r| r.split("  ").next() == Some(code.as_str())).unwrap_or(false)) else {
        eprintln!("agent: {} is not red at {}: nothing to fix", j.test, j.at);
        return 2;
    };
    let mut first = format!(
        "The red test, tests/{}.toml:\n\n{}\nThe spec clause it checks, {binds}:\n\n{clause}\n\nWhat it printed when it ran:\n\n{redline}\n",
        j.test,
        fs::read_to_string(&test_file).unwrap_or_default()
    );
    if let Some(h) = &hint {
        first += &format!("\nFrom your director, who has read how earlier tries went:\n\n{h}");
    }
    let m = Model {
        gear: j.gear,
        name: j.model.clone(),
        url: std::env::var("OLLAMA_URL").ok().filter(|v| !v.is_empty()).unwrap_or_else(|| "http://localhost:11434".into()),
        num_ctx: j.num_ctx,
        max_tokens: j.max_tokens,
        think_cap: j.think_tokens,
        timeout: Duration::from_secs(1800),
    };
    // the conversation's share of the window, less the system prompt; the rest is the reply
    let budget = j.num_ctx * j.room_share / 100;
    let mut b = Builder {
        j: &j,
        dir: dir.clone(),
        base: base.clone(),
        work,
        touched: vec![],
        judged: String::new(),
        by_program: false,
        code,
        budget,
        est: room::Estimate::default(),
        system: system.clone(),
        jig: &jig,
        runs: &runs,
        recorded: recorded.clone(),
        quiet,
        hand: 0,
        paths: vec![],
        result: "refused".into(),
        line: String::new(),
        lines: 0,
        folded: 0,
        jig_down: false,
    };
    say(quiet, &format!("handing {} to {}, {} calls", j.test, j.model, j.max_turns));
    let r = react::run(&m, keys.as_ref(), &system, &first, j.max_turns as u32, &mut b);
    let (ended, why, cannot) = match &r.ended {
        Ended::Finished(how, why) => (how.clone(), why.clone(), false),
        Ended::Turns => ("turns".into(), format!("more than {} calls", j.max_turns), false),
        Ended::Room => ("room".into(), format!("the conversation outgrew the model's context ({} tokens) on call {}", j.num_ctx, r.calls), false),
        Ended::Stopped(why) => ("stopped".into(), why.clone(), false),
        Ended::Unreachable { first_call, why } => ("unreachable".into(), format!("call {}: {why}", r.calls), *first_call),
        Ended::Error(why) => ("error".into(), format!("call {}: {why}", r.calls), false),
        Ended::Halted(how, why) => (how.clone(), why.clone(), r.calls == 0 && how == "budget"),
    };
    if cannot {
        // nothing was spent: not a try, so not recorded
        eprintln!("agent: {why}");
        return 2;
    }
    // A run that ends without a pass, however it ends (out of calls, a reply cut
    // off, out of room or time, the model gone), still has what is on disk handed
    // in and the tests run on it, so there is always a result note to read
    // (findings 0029): unless what is on disk was the last thing judged.
    if ended != "pass" && !b.jig_down && (b.hand == 0 || b.on_disk() != b.judged) {
        say(quiet, &format!("the run ended ({ended}); handing in what is on disk"));
        b.judge_disk(true);
    }
    // the transcript for Chris and the mentor; the note for the director, never the transcript
    let _ = fs::write(dir.join("transcript.json"), serde_json::to_string_pretty(&json!({"system": system, "messages": r.messages})).unwrap_or_default());
    let suite = fs::read_to_string(dir.join("suite.txt")).unwrap_or(before.clone());
    let seconds = start.elapsed().as_secs();
    let n = note::write(&suite, &note::Job { job: &j.job, try_: j.try_, tokens: r.tokens_in + r.tokens_out, seconds, files: &b.paths, calls: Some(r.calls as u64), peak: Some(r.peak) });
    let _ = fs::write(dir.join("note.toml"), &n);
    let dollars = runs.cost(&recorded, r.tokens_in, r.tokens_out).unwrap_or(0.0);
    let run_toml = format!(
        "bench        = \"agent\"\nhanded_in_by = \"{}\"\njob          = {}\ntest         = {}\nat           = \"{}\"\ntry          = {}\nmodel        = {}\nnum_ctx      = {}\nprompt       = \"{}\"\nprompt_print = \"{}\"\nresult       = \"{}\"\nended        = \"{}\"\nwhy          = {}\ncalls        = {}\nhand_ins     = {}\nlines        = {}\ntokens_in    = {}\ntokens_out   = {}\npeak_context = {}\nfolded_chars = {}\nseconds      = {}\ndollars      = {:.6}\n",
        if b.hand == 0 { "nobody" } else if b.by_program { "program" } else { "builder" }, note::toml_string(&j.job, 64), note::toml_string(&j.test, 128), j.at, j.try_, note::toml_string(&recorded, 160), j.num_ctx,
        j.prompt, print, b.result, ended, note::toml_string(&why, 500), r.calls, b.hand, b.lines, r.tokens_in, r.tokens_out,
        r.peak, b.folded, seconds, dollars
    );
    let _ = fs::write(dir.join("run.toml"), run_toml);
    if !dir.join("output.txt").exists() {
        let _ = fs::write(dir.join("output.txt"), "");
    }
    let sitting = std::env::var("AGENT_SITTING").ok().filter(|s| !s.is_empty() && plain(s, "._-"));
    if let Err(e) = runs.record(
        &j.item, &recorded, j.try_, r.tokens_in, r.tokens_out, &b.result, b.lines, &dir.join("transcript.json"),
        &dir.join("output.txt"), &j.at, &print, sitting.as_deref(), r.peak, &ended, &j.trial, r.calls as u64, seconds,
    ) {
        eprintln!("agent: the run could not be recorded in runs/ ({e}); its files are in {}", dir.display());
        return 1;
    }
    println!(
        "{}, try {}, {} at {}: {} ({}); ended: {ended}; {} hand-in(s); {} lines changed; {} in, {} out, peak context {} of {}, over {} calls",
        j.test, j.try_, j.prompt, j.at, b.result, b.line, b.hand, b.lines, r.tokens_in, r.tokens_out, r.peak, j.num_ctx, r.calls
    );
    // a jig that stopped answering is not the builder's try failing: cannot run here
    if b.result == "pass" { 0 } else if b.jig_down { 2 } else { 1 }
}
