// SPDX-License-Identifier: GPL-2.0-only
//! A director: one per part, on the card by default (findings 0027, SPEC.agent.5).
//! It reads the part plan strategic wrote and turns it into jobs, one red test
//! each, for fresh builders; it sees only the result note the program writes after
//! each, never a builder's transcript.  The program keeps the limits: the tries, the
//! minutes, the money and the stuck rule.  Its own conversation is folded when long
//! into a standing the program writes from its log, keeping what findings 0027 says
//! a director's fold must keep: the plan, every job's one-line status and the last
//! three notes whole.

use crate::note;
use crate::plan::{self, Plan};
use crate::record::{self, Runs};
use crate::stuck;
use crate::util::plain;
use runtime::model::{Keys, Model, Stop};
use runtime::react::{self, Ended, Hooks, Step};
use runtime::{prompts, room};
use serde_json::{json, Value};
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

pub const MAX_CALLS: u32 = 200; // the director's own calls, a backstop under the plan's limits
pub const KEEP_NOTES: usize = 3; // result notes a fold keeps whole (findings 0027)
pub const HITS: usize = 40; // lines a search answers with

struct JobDone {
    id: String,
    test: String,
    status: String,
    note: String,
}

fn now() -> u64 {
    SystemTime::now().duration_since(UNIX_EPOCH).map(|d| d.as_secs()).unwrap_or(0)
}

/// A layer's model and context, by gear: the card (AGENT_CARD_MODEL, AGENT_CARD_CTX)
/// or Sonnet on Bedrock (AGENT_SONNET_ID, AGENT_SONNET_CTX).
fn gear(g: u8) -> (String, u64) {
    let get = |k: &str| std::env::var(k).ok().filter(|v| !v.is_empty());
    if g == 2 {
        (get("AGENT_SONNET_ID").unwrap_or_else(|| "global.anthropic.claude-sonnet-4-6".into()),
         get("AGENT_SONNET_CTX").and_then(|v| v.parse().ok()).unwrap_or(200_000))
    } else {
        (get("AGENT_CARD_MODEL").unwrap_or_else(|| "qwen3.8:27b".into()),
         get("AGENT_CARD_CTX").and_then(|v| v.parse().ok()).unwrap_or(98_304))
    }
}

const TOOLS: &str = r#"[{"type":"function","function":{"name":"job","description":"Give one red test from the plan to a fresh builder, with a hint if it helps. You get back its result note.","parameters":{"type":"object","properties":{"test":{"type":"string","description":"A test from the plan, as the plan names it."},"why":{"type":"string","description":"Why this job, now, in one sentence."},"hint":{"type":"string","description":"What the builder should know that it cannot learn from the test: a few sentences at most."}},"required":["test","why"]}}},{"type":"function","function":{"name":"search_log","description":"Find a word in this part's log: every job, hint and result note.","parameters":{"type":"object","properties":{"word":{"type":"string"}},"required":["word"]}}},{"type":"function","function":{"name":"search_prompts","description":"Find a word in the prompt library: the lessons earlier jobs taught.","parameters":{"type":"object","properties":{"word":{"type":"string"}},"required":["word"]}}},{"type":"function","function":{"name":"ask","description":"Stop the part and ask strategic a question only a person can answer.","parameters":{"type":"object","properties":{"question":{"type":"string"}},"required":["question"]}}},{"type":"function","function":{"name":"done","description":"Every test in the plan has passed: report to strategic in a few sentences.","parameters":{"type":"object","properties":{"report":{"type":"string"}},"required":["report"]}}}]"#;

struct Director {
    p: Plan,
    id: String,
    dir: PathBuf,
    repo: PathBuf,
    exe: Vec<String>,
    sitting: String,
    bmodel: String,
    bctx: u64,
    recorded: String,
    runs: Runs,
    jobs: Vec<JobDone>,
    stuck_runs: Vec<stuck::Run>,
    passed: Vec<bool>,
    spent: f64, // the builders' dollars so far
    mine: f64,  // the director's own, as of its last call
    deadline: u64,
    budget: u64,            // the conversation's share of the card, system prompt and tools included
    est: room::Estimate,
    system: String,
    quiet: bool,
    words: String, // the director's own words at the end: a report or a question
}

impl Director {
    fn log(&self, s: &str) {
        use std::io::Write;
        if let Ok(mut f) = fs::OpenOptions::new().create(true).append(true).open(self.dir.join("log.md")) {
            let _ = writeln!(f, "{s}");
        }
    }

    fn say(&self, s: &str) {
        if !self.quiet {
            eprintln!("  {s}");
        }
    }

    /// what is left of the part's dollars: the plan's share, less the builders' and the director's
    fn left(&self) -> f64 {
        (self.p.budget_usd - self.spent - self.mine).max(0.0)
    }

    fn grep(&self, path: &Path, label: &str, word: &str, out: &mut Vec<String>, hits: &mut usize) {
        let Ok(text) = fs::read_to_string(path) else { return };
        for (i, line) in text.lines().enumerate() {
            if line.contains(word) {
                *hits += 1;
                if *hits <= HITS {
                    out.push(format!("{label}:{}: {line}", i + 1));
                }
            }
        }
    }

    /// The fold: everything between the plan and the newest messages becomes one
    /// standing written from the log.  The cut never leaves a tool's answer without
    /// the call it answers.  Checked before it is used: each job's line and each
    /// test must be in it, or the director would forget work it gave (CivicCanada's
    /// fidelity check, with what must be kept decided here).
    fn fold(&self, messages: &mut Vec<Value>) -> Result<bool, String> {
        let fixed = self.est.tokens(&self.system) + self.est.tokens(TOOLS);
        if self.est.size(messages) + fixed <= self.budget || messages.len() <= room::KEEP + 2 {
            return Ok(false);
        }
        let mut cut = messages.len() - room::KEEP;
        while cut > 1 && messages[cut]["role"] == "tool" {
            cut -= 1;
        }
        if cut <= 1 {
            return Ok(false);
        }
        let mut t = String::from("Where the part stands; your older turns were folded to make room.\n\nJobs so far:\n");
        let lines: Vec<String> = self.jobs.iter().map(|j| format!("- {}, {}: {}", j.id, j.test, j.status)).collect();
        for l in &lines {
            t += &format!("{l}\n");
        }
        if self.jobs.is_empty() {
            t += "- none yet\n";
        }
        t += "\nTests in the plan:";
        for (i, test) in self.p.tests.iter().enumerate() {
            t += &format!(" {test} ({})", if self.passed[i] { "passed" } else { "not yet" });
        }
        t += "\n\nThe last result notes, whole:\n";
        for j in self.jobs.iter().skip(self.jobs.len().saturating_sub(KEEP_NOTES)) {
            t += &format!("\n{}\n", j.note);
        }
        let has_line = |l: &str| t.lines().any(|x| x == l);
        if !lines.iter().all(|l| has_line(l)) || !self.p.tests.iter().all(|x| t.contains(&format!(" {x} ("))) {
            return Err("a fold of the director's conversation would have lost a job or a test, so it was not used".into());
        }
        messages.splice(1..cut, [json!({"role": "user", "content": t})]);
        self.log(&format!("fold: {} older messages folded into the part's standing", cut - 1));
        Ok(true)
    }

    /// writes the job, runs a fresh builder on it, and reads back its note.  Err:
    /// the part must stop (how, why).
    fn give(&mut self, test: &str, hint: Option<&str>) -> Result<String, (String, String)> {
        let n = self.jobs.len() + 1;
        let id = format!("{}-{n}", self.id);
        let jobs = self.dir.join("jobs");
        let jobfile = jobs.join(format!("{id}.toml"));
        let mut job = format!(
            "# written by the director for {}, from the part plan\njob = \"{id}\"\ntest = \"{test}\"\nat = \"{}\"\ntry = {n}\nprompt = \"{}\"\nmodel = \"{}\"\nnum_ctx = {}\ndeadline = {}\n",
            self.id, self.p.at, self.p.prompt_builder, self.bmodel, self.bctx, self.deadline
        );
        if self.p.gear_builders == 2 {
            // a paid builder may spend what is left of the part's share
            job += &format!("gear = 2\nmax_usd = {:.6}\n", self.left());
        }
        if let Some(h) = hint.filter(|h| !h.trim().is_empty()) {
            let hp = jobs.join(format!("{id}.hint"));
            let _ = fs::write(&hp, format!("{h}\n"));
            job += &format!("hint = {}\n", note::toml_string(&hp.to_string_lossy(), 1000));
        }
        if fs::write(&jobfile, &job).is_err() {
            return Err(("error".into(), "the job could not be written".into()));
        }
        self.log(&format!("\n## {id}: {test}\n{}", hint.map(|h| format!("hint: {h}")).unwrap_or_else(|| "no hint".into())));
        self.say(&format!("job {id}: {test}{}", if hint.is_some() { ", with a hint" } else { "" }));
        let status = Command::new(&self.exe[0])
            .args(&self.exe[1..])
            .arg("build")
            .arg(&jobfile)
            .env("AGENT_OUT", &jobs)
            .env("AGENT_SITTING", &self.sitting)
            .stdout(fs::File::create(jobs.join(format!("{id}.out"))).map_err(|e| ("error".to_string(), e.to_string()))?)
            .stderr(fs::File::create(jobs.join(format!("{id}.err"))).map_err(|e| ("error".to_string(), e.to_string()))?)
            .status();
        let rc = status.ok().and_then(|s| s.code()).unwrap_or(2);
        let run_dir = jobs.join(format!("{id}-try{n}"));
        let run = fs::read_to_string(run_dir.join("run.toml")).ok().and_then(|t| toml::from_str::<toml::Table>(&t).ok());
        if let Some(r) = &run {
            self.spent += r.get("dollars").and_then(|v| v.as_float()).unwrap_or(0.0);
        }
        if rc == 2 {
            let err = fs::read_to_string(jobs.join(format!("{id}.err"))).unwrap_or_default();
            let first = err.lines().find(|l| !l.trim().is_empty()).unwrap_or("").trim().to_string();
            return Err(("cannot run".into(), format!("the builder for {id} could not run: {first}")));
        }
        let (Some(r), Ok(nt)) = (run, fs::read_to_string(run_dir.join("note.toml"))) else {
            return Err(("cannot run".into(), format!("the builder for {id} left no run record or note in {}", run_dir.display())));
        };
        let s = |k: &str| r.get(k).and_then(|v| v.as_str()).unwrap_or("").to_string();
        let passed = s("result") == "pass";
        let status = format!("{}, ended {}{}{}", s("result"), s("ended"), if s("why").is_empty() { "" } else { ": " }, s("why"));
        let Some(sr) = stuck::from_note(&nt, test, passed) else {
            return Err(("error".into(), format!("the note for {id} could not be read")));
        };
        self.stuck_runs.push(sr);
        if passed {
            if let Some(i) = self.p.tests.iter().position(|t| t == test) {
                self.passed[i] = true;
            }
        }
        self.log(&format!("result: {status}\n```\n{nt}```"));
        self.say(&format!("job {id}: {status}"));
        let answer = format!("Job {id}, {test}: {status}.\nIts result note, written by the program from what the tests printed on the jig:\n{nt}");
        self.jobs.push(JobDone { id, test: test.into(), status, note: nt });
        Ok(answer)
    }
}

impl Hooks for Director {
    fn tools(&self) -> Vec<Value> {
        serde_json::from_str(TOOLS).expect("the director's tools are JSON")
    }

    fn before(&mut self, turn: u32, messages: &mut Vec<Value>, tin: u64, tout: u64) -> Result<(), (String, String)> {
        if now() >= self.deadline {
            return Err(("out of time".into(), format!("the plan's {} minutes were used", self.p.max_minutes)));
        }
        self.fold(messages).map_err(|why| ("error".to_string(), why))?;
        if self.p.gear_tactical == 2 {
            // a paid call: the part's share first, then the month's cap (decision 0095)
            self.mine = self.runs.cost(&self.recorded, tin, tout).unwrap_or(0.0);
            let next = crate::build::max_call();
            if self.left() < next {
                return Err(("budget".into(), format!("before call {turn}: the part's ${:.2} has ${:.4} left, less than one call may cost", self.p.budget_usd, self.left())));
            }
            self.runs.room(self.mine + next).map_err(|why| ("budget".to_string(), format!("before call {turn}: {why}")))?;
        }
        Ok(())
    }

    fn call(&mut self, name: &str, args: &Value) -> Step {
        let s = |k: &str| args[k].as_str().map(str::trim).filter(|v| !v.is_empty()).map(String::from);
        match name {
            "job" => {
                let Some(_why) = s("why") else {
                    // reason, then act, as a builder's look does (decision 0092)
                    return Step::Answer("refused: say why this job, now, in why.  No builder was started.".into());
                };
                let test = s("test").unwrap_or_default();
                if !self.p.tests.contains(&test) {
                    return Step::Answer(format!("refused: {} is not a test in the plan.  The plan's tests: {}", if test.is_empty() { "(no test)" } else { &test }, self.p.tests.join(" ")));
                }
                if self.jobs.len() as u64 >= self.p.max_tries {
                    let why = format!("all {} tries in the plan were used", self.p.max_tries);
                    return Step::Finish(format!("refused: {why}"), "out of tries".into(), why);
                }
                if now() >= self.deadline {
                    let why = format!("the plan's {} minutes were used", self.p.max_minutes);
                    return Step::Finish(format!("refused: {why}"), "out of time".into(), why);
                }
                if self.p.gear_builders == 2 && self.left() < crate::build::max_call() {
                    let why = format!("the part's ${:.2} has ${:.4} left, less than one builder call may cost", self.p.budget_usd, self.left());
                    return Step::Finish(format!("refused: {why}"), "budget".into(), why);
                }
                let hint = s("hint");
                match self.give(&test, hint.as_deref()) {
                    Err((how, why)) => Step::Finish(why.clone(), how, why),
                    Ok(answer) => match stuck::check(&self.stuck_runs, self.p.stuck_same_failures, self.p.stuck_no_drop) {
                        Some(why) => Step::Finish(format!("{answer}\nThe part is stuck: {why}.  It stops here and goes to strategic."), "stuck".into(), why),
                        None => Step::Answer(answer),
                    },
                }
            }
            "search_log" | "search_prompts" => {
                let Some(word) = s("word") else { return Step::Answer("search needs a word".into()) };
                let (mut out, mut hits) = (Vec::new(), 0);
                if name == "search_log" {
                    self.grep(&self.dir.join("log.md"), "log", &word, &mut out, &mut hits);
                } else {
                    for p in [&self.p.prompt_builder, &self.p.prompt_director] {
                        self.grep(&self.repo.join(format!("prompts/{p}.txt")), p, &word, &mut out, &mut hits);
                    }
                }
                if hits == 0 {
                    Step::Answer(format!("no line holds {word}"))
                } else {
                    let more = if hits > HITS { format!("\n{} more lines; narrow the word", hits - HITS) } else { String::new() };
                    Step::Answer(out.join("\n") + &more)
                }
            }
            "ask" => {
                self.words = s("question").unwrap_or_default();
                Step::Finish("Asked.  The part waits for strategic.".into(), "asked".into(), "the director asked strategic a question".into())
            }
            "done" => {
                // the program checks done, from the notes; the director cannot say so
                let not_yet: Vec<&str> = self.p.tests.iter().zip(&self.passed).filter(|(_, p)| !**p).map(|(t, _)| t.as_str()).collect();
                if !not_yet.is_empty() {
                    return Step::Answer(format!("refused: not every test in the plan has passed.  Not yet: {}", not_yet.join(" ")));
                }
                self.words = s("report").unwrap_or_default();
                Step::Finish("Done.".into(), "done".into(), format!("every test in the plan passed, in {} job(s)", self.jobs.len()))
            }
            other => Step::Answer(format!("There is no tool {other}.  The tools are job, search_log, search_prompts, ask and done.")),
        }
    }

    fn after(&mut self, _turn: u32, _left: u32) -> Option<String> {
        None
    }

    fn nudge(&mut self, _stop: Stop, _turn: u32, _left: u32) -> String {
        "You stopped without a tool.  Give a job, search, ask strategic, or report done.".into()
    }

    fn sent(&mut self, counted: u64, messages: &[Value]) {
        let plain = room::tokens(&self.system) + room::tokens(TOOLS) + room::size(messages);
        self.est.calibrate(counted, plain);
    }

    fn tally(&mut self, tin: u64, tout: u64) {
        // the director's own dollars, as of the reply it is acting on
        self.mine = self.runs.cost(&self.recorded, tin, tout).unwrap_or(0.0);
    }
}

fn absolute(p: &str) -> PathBuf {
    let p = PathBuf::from(p);
    if p.is_absolute() { p } else { std::env::current_dir().unwrap_or_default().join(p) }
}

/// Runs the part; 0 done, 1 stopped with a question note, 2 cannot run.
pub fn run(planpath: &str) -> i32 {
    let quiet = std::env::var_os("AGENT_QUIET").is_some();
    let started = now();
    let p = match plan::read(planpath) {
        Ok(p) => p,
        Err(why) => {
            eprintln!("agent: plan {planpath}: {why}");
            return 2;
        }
    };
    // the part's name is the plan file's: agent/plans/waits.toml is the part waits
    let id: String = Path::new(planpath).file_name().map(|f| f.to_string_lossy().to_string()).unwrap_or_default();
    let id = id.split('.').next().unwrap_or("").chars().take(60).collect::<String>();
    if id.is_empty() || !plain(&id, "_-") {
        eprintln!("agent: a plan's file name is letters, digits, _ and - before .toml, not {id}");
        return 2;
    }
    if (p.gear_tactical == 2 || p.gear_builders == 2) && p.budget_usd <= 0.0 {
        // a paid layer with no share of the month cannot make a call, so it does not start
        eprintln!("agent: plan {planpath}: gear 2 costs money, and the plan's budget_usd is 0");
        return 2;
    }
    let (model, num_ctx) = gear(p.gear_tactical);
    let (bmodel, bctx) = gear(p.gear_builders);
    if !plain(&model, "._-:/") || !plain(&bmodel, "._-:/") || num_ctx < 2048 || bctx < 2048 {
        eprintln!("agent: AGENT_CARD_MODEL, AGENT_SONNET_ID or their context is not one");
        return 2;
    }
    let repo = absolute(&std::env::var("AGENT_REPO").ok().filter(|v| !v.is_empty()).unwrap_or_else(|| ".".into()));
    let out = absolute(&std::env::var("AGENT_OUT").ok().filter(|v| !v.is_empty()).unwrap_or_else(|| "out/agent".into()));
    let dir = out.join(&id);
    // the builders are this program, run fresh for each job; AGENT_BUILDER stands in for tests
    let exe: Vec<String> = match std::env::var("AGENT_BUILDER") {
        Ok(v) if !v.trim().is_empty() => v.split_whitespace().map(String::from).collect(),
        _ => match std::env::current_exe() {
            Ok(e) => vec![e.to_string_lossy().to_string()],
            Err(_) => {
                eprintln!("agent: cannot find this program to run a builder");
                return 2;
            }
        },
    };
    let (system, print) = match prompts::load(&repo.join("prompts"), &p.prompt_director) {
        Ok(x) => x,
        Err(why) => {
            eprintln!("agent: {why}");
            return 2;
        }
    };
    let recorded = format!("{}:{model}", if p.gear_tactical == 2 { "bedrock" } else { "ollama" });
    let runs = Runs::new(&repo);
    let keys: Option<Keys> = if p.gear_tactical == 2 {
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
    let _ = fs::remove_dir_all(&dir);
    if fs::create_dir_all(dir.join("jobs")).is_err() {
        eprintln!("agent: could not make {}", dir.display());
        return 2;
    }
    let stamp = runtime::sigv4::now();
    let plan_text = fs::read_to_string(planpath).unwrap_or_default();
    let first = format!("The part plan, {id}:\n\n{plan_text}");
    let budget = num_ctx * 60 / 100;
    let mut d = Director {
        passed: vec![false; p.tests.len()],
        deadline: started + p.max_minutes * 60,
        p,
        id: id.clone(),
        dir: dir.clone(),
        repo,
        exe,
        sitting: stamp.clone(),
        bmodel,
        bctx,
        recorded: recorded.clone(),
        runs,
        jobs: vec![],
        stuck_runs: vec![],
        spent: 0.0,
        mine: 0.0,
        budget,
        est: room::Estimate::default(),
        system: system.clone(),
        quiet,
        words: String::new(),
    };
    d.log(&format!("# {id}\n\nplan {planpath}, at {}, sitting {stamp}", d.p.at));
    d.say(&format!("directing {id}: {} test(s), {} tries, {} minutes", d.p.tests.len(), d.p.max_tries, d.p.max_minutes));
    let m = Model {
        gear: d.p.gear_tactical,
        name: model,
        url: std::env::var("OLLAMA_URL").ok().filter(|v| !v.is_empty()).unwrap_or_else(|| "http://localhost:11434".into()),
        num_ctx,
        max_tokens: 8000.min(num_ctx / 8),
        think_cap: 8000.min(num_ctx / 8),
        timeout: Duration::from_secs(1800),
    };
    let r = react::run(&m, keys.as_ref(), &system, &first, MAX_CALLS, &mut d);
    let (how, why) = match &r.ended {
        Ended::Finished(how, why) => (how.clone(), why.clone()),
        Ended::Turns => ("stopped".into(), format!("the director made {MAX_CALLS} calls")),
        Ended::Room => ("room".into(), format!("the director's conversation outgrew the card ({num_ctx} tokens) on call {}", r.calls)),
        Ended::Stopped(why) => ("stopped".into(), format!("the director {why}, twice")),
        Ended::Unreachable { first_call: true, why } => {
            eprintln!("agent: {why}");
            return 2;
        }
        Ended::Unreachable { why, .. } => ("unreachable".into(), format!("call {}: {why}", r.calls)),
        Ended::Error(why) => ("error".into(), format!("call {}: {why}", r.calls)),
        Ended::Halted(how, why) => (how.clone(), why.clone()),
    };
    d.log(&format!("\n{how}: {why}"));
    // the part report, and for anything but done the question note for strategic
    let minutes = (now() - started).div_ceil(60);
    let mut report = format!(
        "# {id}: {how}\n\n{why}\n\nPlan: {}, at {}, director {} ({print}), builders {}.\nDirector: {} calls, {} tokens in, {} out, peak context {} of {num_ctx}; {minutes} minutes.\n\n",
        d.p.part, d.p.at, d.p.prompt_director, d.p.prompt_builder, r.calls, r.tokens_in, r.tokens_out, r.peak
    );
    for j in &d.jobs {
        report += &format!("- {}, {}: {}\n", j.id, j.test, j.status);
    }
    if !d.words.is_empty() {
        report += &format!("\nThe director's words:\n\n{}\n", d.words);
    }
    let _ = fs::write(dir.join("report.md"), &report);
    if how != "done" {
        let mut q = format!("# {id} stopped: {how}\n\n{why}\n");
        if how == "asked" {
            q += &format!("\nThe director asks:\n\n{}\n", d.words);
        }
        q += &format!(
            "\nTries used: {} of {}.  Failing counts, oldest first: {}.\n\nThe last result notes:\n",
            d.jobs.len(),
            d.p.max_tries,
            d.stuck_runs.iter().map(|r| r.failed.to_string()).collect::<Vec<_>>().join(", ")
        );
        for j in d.jobs.iter().skip(d.jobs.len().saturating_sub(KEEP_NOTES)) {
            q += &format!("\n{}:\n```\n{}```\n", j.id, j.note);
        }
        let _ = fs::write(dir.join("question.md"), q);
    }
    let _ = fs::write(dir.join("director.json"), serde_json::to_string_pretty(&json!({"system": system, "messages": r.messages})).unwrap_or_default());
    // the director's own run into runs/ too, so its tokens and dollars are counted
    let ended = match how.as_str() {
        "done" => "pass",
        "stuck" | "room" | "budget" | "unreachable" | "error" => how.as_str(),
        "out of time" => "time",
        _ => "stopped",
    };
    let item = format!("direct-{}", id.to_lowercase());
    if let Err(e) = d.runs.record(
        &item, &recorded, 1, r.tokens_in, r.tokens_out, if how == "done" { "pass" } else { "fail" }, 0,
        &dir.join("director.json"), &dir.join("report.md"), &d.p.at, &print, Some(&stamp), r.peak, ended,
    ) {
        eprintln!("agent: the director's run could not be recorded in runs/ ({e}); its files are in {}", dir.display());
    }
    println!(
        "{id}: {how} ({why}); {} job(s); director {} calls, {} in, {} out, peak context {} of {num_ctx}",
        d.jobs.len(), r.calls, r.tokens_in, r.tokens_out, r.peak
    );
    match how.as_str() {
        "done" => 0,
        "cannot run" => 2,
        _ => 1,
    }
}
