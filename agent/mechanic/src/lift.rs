// SPDX-License-Identifier: GPL-2.0-only
//! A lifter: a builder whose job is an exam item, not a red test (findings 0030).
//! It is given what jig/sit gives a model sitting the item: the question, the name
//! of the C file to write and the fixed header, and Linux only through the shared
//! source reader on the jig.  It never sees the repo, so it never sees the hand
//! lift.  It writes the file and hands it in; the item's tests run on the jig with
//! the lift in the hand lift's place, and every one of them must print PASS, which
//! is jig/judge's rule.  The loop, the limits, the hand-in at the end and the
//! record are the builder's (build.rs), so the race differs only in the bench.

use crate::build::{absolute, max_call, now, say, Job};
use crate::judge::{self, Jig};
use crate::note;
use crate::record::{self, Runs};
use crate::util::plain;
use runtime::model::{Keys, Model, Stop};
use runtime::react::{self, Ended, Hooks, Step};
use runtime::{prompts, room};
use serde_json::{json, Value};
use std::fs;
use std::path::PathBuf;
use std::process::Command;
use std::time::{Duration, Instant};

const TOOLS: &str = r#"[{"type":"function","function":{"name":"read_linux","description":"Read Linux, one fixed copy of it, through the shared source reader. command is list (args: a folder, \".\" for the top), read (args: a file, the first line and the last line), find (args: a name: where it is defined), callers (args: a name), calls (args: a name), ports (args: a file, first and last line: the lines that read or write an I/O port) or check (args: a file, first and last line).","parameters":{"type":"object","properties":{"why":{"type":"string","description":"What you expect to learn from this read, in one sentence. A read without it is refused."},"command":{"type":"string","enum":["list","read","find","callers","calls","ports","check"]},"args":{"type":"array","items":{"type":"string"}}},"required":["why","command","args"]}}},{"type":"function","function":{"name":"write_file","description":"Write the C file the part asks for, whole: name is the file's name, exactly as you were given it, and content its whole text. Writing it again replaces it. Nothing is judged until you hand it in.","parameters":{"type":"object","properties":{"name":{"type":"string"},"content":{"type":"string"}},"required":["name","content"]}}},{"type":"function","function":{"name":"submit_lift","description":"Hand in the file you wrote. The part's tests run with it in place: every one must pass. A pass ends the run; otherwise you are told what failed, the file stays as you wrote it, and you may write it again and hand it in while you have calls left. A run that ends without a hand-in has what you last wrote handed in for it.","parameters":{"type":"object","properties":{}}}}]"#;

/// One item of exam/items.toml, as the job's commit has it.
struct Item {
    asks: String,
    names: Vec<String>, // the .c files to write, by name
    hand: Vec<String>,  // the hand lift's .c paths, where a lift goes to be judged
    headers: Vec<String>,
    codes: Vec<String>, // the tests, TST.name@N
    suite: String,      // the tests as jig/test names them
}

fn item(base: &std::path::Path, name: &str) -> Result<Item, String> {
    let text = fs::read_to_string(base.join("exam/items.toml")).map_err(|_| "no exam/items.toml at the job's commit".to_string())?;
    let t: toml::Table = toml::from_str(&text).map_err(|_| "exam/items.toml could not be read".to_string())?;
    let items = t.get("item").and_then(|v| v.as_array()).ok_or("exam/items.toml has no items")?;
    let it = items
        .iter()
        .find(|i| i.get("name").and_then(|v| v.as_str()) == Some(name))
        .ok_or_else(|| format!("no item {name} in exam/items.toml"))?;
    let s = |k: &str| it.get(k).and_then(|v| v.as_str()).unwrap_or("").to_string();
    let (asks, hand, tests) = (s("asks"), s("hand"), s("tests"));
    let mut it = Item { asks, names: vec![], hand: vec![], headers: vec![], codes: vec![], suite: tests.clone() };
    for f in hand.split_whitespace() {
        let ok = plain(f, "/._-@") && !f.split('/').any(|p| p.is_empty() || p == "." || p == "..");
        if !ok {
            return Err(format!("item {name} names a file a job may not use: {f}"));
        }
        if f.ends_with(".c") {
            it.names.push(f.rsplit('/').next().unwrap_or(f).to_string());
            it.hand.push(f.to_string());
        } else if f.ends_with(".h") {
            it.headers.push(f.to_string());
        }
    }
    for t in tests.split_whitespace() {
        if !plain(t, "._-@") {
            return Err(format!("item {name} names a test a job may not run: {t}"));
        }
        it.codes.push(format!("TST.{t}"));
    }
    if it.asks.is_empty() || it.names.is_empty() || it.codes.is_empty() {
        return Err(format!("item {name} has no question, no .c file or no tests"));
    }
    Ok(it)
}

struct Lifter<'a> {
    j: &'a Job,
    it: Item,
    dir: PathBuf,
    written: Vec<String>, // names written, in the order first written
    judged: String,
    by_program: bool,
    budget: u64,
    est: room::Estimate,
    system: String,
    jig: &'a Jig,
    runs: &'a Runs,
    recorded: String,
    quiet: bool,
    hand: u64,
    result: String,
    line: String,
    lines: u64,
    folded: usize,
    jig_down: bool,
}

impl Lifter<'_> {
    fn lift_dir(&self) -> PathBuf {
        self.dir.join("lift")
    }

    fn on_disk(&self) -> String {
        self.written.iter().map(|n| format!("{n}\n{}", fs::read_to_string(self.lift_dir().join(n)).unwrap_or_default())).collect()
    }

    fn write(&mut self, args: &Value) -> String {
        let (Some(name), Some(content)) = (args["name"].as_str(), args["content"].as_str()) else {
            return "write_file needs name and content.  Nothing was written.".into();
        };
        if !self.it.names.iter().any(|n| n == name) {
            return format!("refused: the part asks for {}, not {name}.  Nothing was written.", self.it.names.join(" "));
        }
        let _ = fs::create_dir_all(self.lift_dir());
        if fs::write(self.lift_dir().join(name), content).is_err() {
            return format!("{name} could not be written.");
        }
        if !self.written.iter().any(|n| n == name) {
            self.written.push(name.to_string());
        }
        say(self.quiet, &format!("write {name}, {} lines", content.lines().count()));
        format!("Wrote {name}.  Nothing is judged until you hand it in with submit_lift; a run that ends without a hand-in has what you last wrote handed in for it.")
    }

    /// the lift, in the hand lift's place, against the item's tests on the jig
    fn judge(&mut self, by_program: bool) -> String {
        self.hand += 1;
        let h = self.hand;
        let fixed = self.dir.join("fixed");
        let _ = fs::remove_dir_all(&fixed);
        let mut output = String::new();
        let mut files = Vec::new();
        for (name, path) in self.it.names.iter().zip(&self.it.hand) {
            let from = self.lift_dir().join(name);
            let Ok(text) = fs::read_to_string(&from) else { continue };
            let to = fixed.join(path);
            if let Some(p) = to.parent() {
                let _ = fs::create_dir_all(p);
            }
            let _ = fs::write(&to, &text);
            output += &format!("=== {name} ===\n{text}");
            files.push(from);
        }
        let _ = fs::write(self.dir.join("output.txt"), &output);
        self.judged = self.on_disk();
        self.by_program = by_program;
        self.lines = self.runs.count(&files).unwrap_or(0);
        let missing: Vec<&String> = self.it.names.iter().filter(|n| !self.written.contains(n)).collect();
        if !missing.is_empty() {
            // jig/sit's rule: a lift without every file asked for is refused, not run
            self.result = "refused".into();
            self.line = format!("no {}", missing.iter().map(|s| s.as_str()).collect::<Vec<_>>().join(" "));
            return format!("Hand-in {h} refused: the part asks for {} and you have not written it.", self.line.trim_start_matches("no "));
        }
        say(self.quiet, &format!("hand-in {h}{}, {} lines; running the part's tests on the jig", if by_program { ", by the program, from what was written" } else { "" }, self.lines));
        let after = match self.jig.run(&self.j.at, &self.it.suite, Some(&fixed)) {
            Ok(a) => a,
            Err(why) => {
                self.jig_down = true;
                self.result = "fail".into();
                self.line = why.clone();
                let _ = fs::write(self.dir.join("suite-did-not-run.txt"), &why);
                return format!("Hand-in {h} was not judged: {why}.");
            }
        };
        let _ = fs::write(self.dir.join("suite.txt"), &after);
        match judge::lift_verdict(&after, &self.it.codes) {
            None => {
                self.result = "pass".into();
                self.line = format!("every one of the part's {} tests passed", self.it.codes.len());
                say(self.quiet, &format!("hand-in {h}: pass"));
                format!("Hand-in {h} judged: pass.  {}.", self.line)
            }
            Some(red) => {
                self.result = "fail".into();
                self.line = red.clone();
                say(self.quiet, &format!("hand-in {h}: fail: {}", runtime::model::cut(&red, 140)));
                format!("Hand-in {h} judged: fail.  {red}\nWhat you wrote is kept.  Write it again with write_file and hand it in with submit_lift.")
            }
        }
    }
}

impl Hooks for Lifter<'_> {
    fn tools(&self) -> Vec<Value> {
        serde_json::from_str(TOOLS).expect("the lifter's tools are JSON")
    }

    fn before(&mut self, turn: u32, messages: &mut Vec<Value>, tin: u64, tout: u64) -> Result<(), (String, String)> {
        if self.jig_down {
            return Err(("error".into(), format!("the part's tests did not run on the jig on hand-in {}", self.hand)));
        }
        if let Some(d) = self.j.deadline {
            if now() >= d {
                return Err(("time".into(), format!("before call {turn}: the part's minutes are used")));
            }
        }
        let fixed = self.est.tokens(&self.system) + self.est.tokens(TOOLS);
        let dropped = room::drop_old_measured(messages, self.budget.saturating_sub(fixed), &self.est);
        if dropped > 0 {
            self.folded += dropped;
            say(self.quiet, &format!("call {turn}: {dropped} characters of old answers dropped to make room"));
        }
        if self.j.gear == 2 {
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
            "read_linux" => {
                let why = args["why"].as_str().unwrap_or("").trim();
                if why.is_empty() {
                    return Step::Answer("refused: say why you are reading, in why: what you expect to learn from this read, in one sentence.  Nothing was read.".into());
                }
                let cmd = args["command"].as_str().unwrap_or("");
                let a: Vec<String> = args["args"]
                    .as_array()
                    .into_iter()
                    .flatten()
                    .filter_map(|v| v.as_str().map(String::from).or_else(|| v.as_u64().map(|n| n.to_string())))
                    .collect();
                say(self.quiet, &format!("read_linux {cmd} {}; why: {}", a.join(" "), runtime::model::cut(why, 120)));
                match self.jig.reader(cmd, &a) {
                    Ok((text, _)) => Step::Answer(room::cap(&text, self.j.look_chars as usize)),
                    Err(why) => Step::Answer(format!("refused: {why}.  Nothing was read.")),
                }
            }
            "write_file" => Step::Answer(self.write(args)),
            "submit_lift" => {
                if self.written.is_empty() {
                    return Step::Answer(format!("Nothing is written yet: write {} with write_file, then hand it in.", self.it.names.join(" ")));
                }
                let a = self.judge(false);
                if self.result == "pass" {
                    Step::Finish(a, "pass".into(), String::new())
                } else {
                    Step::Answer(a)
                }
            }
            other => Step::Answer(format!("There is no tool {other}.  The tools are read_linux, write_file and submit_lift.")),
        }
    }

    fn after(&mut self, turn: u32, left: u32) -> Option<String> {
        let mut n = format!("Call {turn} of {} done; {left} left.", self.j.max_turns);
        if left <= 10 {
            n += "  Ten or fewer left: write the file and hand it in with submit_lift now.  A run that ends without one has what you last wrote handed in for it.";
        }
        Some(n)
    }

    fn sent(&mut self, counted: u64, messages: &[Value]) {
        let plain = room::tokens(&self.system) + room::tokens(TOOLS) + room::size(messages);
        self.est.calibrate(counted, plain);
    }

    fn nudge(&mut self, stop: Stop, turn: u32, left: u32) -> String {
        let what = if stop == Stop::MaxTokens {
            "Your reply was cut off at its limit.  Keep the next reply shorter."
        } else if self.written.is_empty() {
            "You stopped without writing the file."
        } else {
            "You stopped without handing in what you wrote."
        };
        format!("{what}  Call {turn} of {} done; {left} left.  Read Linux further if you need to, write the file with write_file and hand it in with submit_lift.", self.j.max_turns)
    }
}

/// Runs a lift job; 0 a pass, 1 not a pass, 2 cannot run here.
pub fn run(j: &Job, start: Instant, quiet: bool) -> i32 {
    let repo = absolute(&std::env::var("AGENT_REPO").ok().filter(|v| !v.is_empty()).unwrap_or_else(|| ".".into()));
    let out = absolute(&std::env::var("AGENT_OUT").ok().filter(|v| !v.is_empty()).unwrap_or_else(|| "out/agent".into()));
    let dir = out.join(format!("{}-try{}", j.job, j.try_));
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
    // the item and its header as the job's commit has them: the program reads them,
    // the lifter is only told them
    let _ = fs::remove_dir_all(&dir);
    let base = dir.join("base");
    let _ = fs::create_dir_all(&base);
    let copied = Command::new("sh")
        .arg("-c")
        .arg("git -C \"$1\" archive \"$2\" exam lifted | tar -xf - -C \"$3\"")
        .args(["copy", &repo.to_string_lossy(), &j.at, &base.to_string_lossy()])
        .status()
        .map(|s| s.success())
        .unwrap_or(false);
    if !copied {
        eprintln!("agent: could not read exam/ and lifted/ at {}", j.at);
        return 2;
    }
    let it = match item(&base, &j.exam) {
        Ok(i) => i,
        Err(why) => {
            eprintln!("agent: {why}");
            return 2;
        }
    };
    // before spending anything, as jig/sit does: the hand lift passes the part's tests
    // on the jig, so a lift could pass them too
    let jig = Jig::from_env();
    say(quiet, &format!("running {}'s tests on the jig at {} with the hand lift, to see that a lift can pass", j.exam, j.at));
    let before = match jig.run(&j.at, &it.suite, None) {
        Ok(b) => b,
        Err(why) => {
            eprintln!("agent: {why}");
            return 2;
        }
    };
    let _ = fs::write(dir.join("before.txt"), &before);
    if let Some(red) = judge::lift_verdict(&before, &it.codes) {
        eprintln!("agent: the hand lift of {} does not pass at {}, so no lift could: {red}", j.exam, j.at);
        return 2;
    }
    // what jig/sit's first message says: the question, the file to write, the header
    let mut first = format!("Part to lift: {}\n\nName of the C file to write: {}\n", it.asks, it.names.join(" "));
    for h in &it.headers {
        let name = h.rsplit('/').next().unwrap_or(h);
        first += &format!("\nThe fixed header {name}:\n\n{}", fs::read_to_string(base.join(h)).unwrap_or_default());
    }
    if let Some(h) = &hint {
        first += &format!("\nFrom your director, who has read how earlier tries went:\n\n{h}");
    }
    // the hand lift itself is gone before the lifter's first call, so nothing it is
    // shown could come from it
    let _ = fs::remove_dir_all(base.join("lifted"));
    let m = Model {
        gear: j.gear,
        name: j.model.clone(),
        url: std::env::var("OLLAMA_URL").ok().filter(|v| !v.is_empty()).unwrap_or_else(|| "http://localhost:11434".into()),
        num_ctx: j.num_ctx,
        max_tokens: j.max_tokens,
        think_cap: j.think_tokens,
        timeout: Duration::from_secs(1800),
    };
    let mut b = Lifter {
        j,
        it,
        dir: dir.clone(),
        written: vec![],
        judged: String::new(),
        by_program: false,
        budget: j.num_ctx * j.room_share / 100,
        est: room::Estimate::default(),
        system: system.clone(),
        jig: &jig,
        runs: &runs,
        recorded: recorded.clone(),
        quiet,
        hand: 0,
        result: "refused".into(),
        line: String::new(),
        lines: 0,
        folded: 0,
        jig_down: false,
    };
    say(quiet, &format!("handing the {} lift to {}, {} calls", j.exam, j.model, j.max_turns));
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
        eprintln!("agent: {why}");
        return 2;
    }
    // what was written and never handed in is handed in for it, as a builder's is
    if ended != "pass" && !b.jig_down && !b.written.is_empty() && (b.hand == 0 || b.on_disk() != b.judged) {
        say(quiet, &format!("the run ended ({ended}); handing in what was written"));
        b.judge(true);
    }
    let _ = fs::write(dir.join("transcript.json"), serde_json::to_string_pretty(&json!({"system": system, "messages": r.messages})).unwrap_or_default());
    // the note from the part's tests as they last ran, or the hand lift's run if none did
    let suite = fs::read_to_string(dir.join("suite.txt")).unwrap_or_else(|_| {
        b.it.codes.iter().map(|c| format!("FAIL  {c}  0s  nothing was handed in\n")).collect()
    });
    let seconds = start.elapsed().as_secs();
    let files: Vec<String> = b.written.iter().map(|n| format!("lift/{n}")).collect();
    let n = note::write(&suite, &note::Job { job: &j.job, try_: j.try_, tokens: r.tokens_in + r.tokens_out, seconds, files: &files, calls: Some(r.calls as u64), peak: Some(r.peak) });
    let _ = fs::write(dir.join("note.toml"), &n);
    let dollars = runs.cost(&recorded, r.tokens_in, r.tokens_out).unwrap_or(0.0);
    let run_toml = format!(
        "bench        = \"agent\"\nhanded_in_by = \"{}\"\njob          = {}\nexam         = {}\nat           = \"{}\"\ntry          = {}\nmodel        = {}\nnum_ctx      = {}\nprompt       = \"{}\"\nprompt_print = \"{}\"\nresult       = \"{}\"\nended        = \"{}\"\nwhy          = {}\nline         = {}\ncalls        = {}\nhand_ins     = {}\nlines        = {}\ntokens_in    = {}\ntokens_out   = {}\npeak_context = {}\nfolded_chars = {}\nseconds      = {}\ndollars      = {:.6}\n",
        if b.hand == 0 { "nobody" } else if b.by_program { "program" } else { "builder" }, note::toml_string(&j.job, 64), note::toml_string(&j.exam, 64), j.at, j.try_, note::toml_string(&recorded, 160), j.num_ctx,
        j.prompt, print, b.result, ended, note::toml_string(&why, 500), note::toml_string(&b.line, 300), r.calls, b.hand, b.lines, r.tokens_in, r.tokens_out,
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
        "{}, try {}, {} at {}: {} ({}); ended: {ended}; {} hand-in(s); {} lines; {} in, {} out, peak context {} of {}, over {} calls",
        j.exam, j.try_, j.prompt, j.at, b.result, b.line, b.hand, b.lines, r.tokens_in, r.tokens_out, r.peak, j.num_ctx, r.calls
    );
    if b.result == "pass" { 0 } else if b.jig_down { 2 } else { 1 }
}
