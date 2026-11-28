// SPDX-License-Identifier: GPL-2.0-only
//! Call a friend (findings 0052, 0053, 0055): when the card is out of its depth,
//! Claude, on Bedrock, is shown what the card was given, what it has done and
//! what the test says now, looks at the card's copy of the repo for itself, a few
//! looks at most, and sends back a briefing: what is actually wrong, where to
//! look, a few lines of example, what to do first.  Every place it points at is
//! a file and lines quoted from it, and a briefing whose quote is not in the
//! file is refused, so it cannot brief from a guess.  Only the briefing reaches
//! the card; what the friend said on the way to it does not.  It writes no fix.
//! Each turn is asked of the friend's share and the month's cap first, and the
//! whole call is recorded in runs/, so the month's spend counts it.

use crate::look;
use crate::record::{self, Runs};
use runtime::model::{Model, Stop};
use runtime::react::{self, Ended, Hooks, Step};
use runtime::prompts;
use serde_json::Value;
use std::fs;
use std::path::{Path, PathBuf};
use std::time::Duration;

pub const PROMPT: &str = "friend@3";
pub const LOOKS: u32 = 10;
const LOOK_CHARS: usize = 8_000;

pub struct Briefing {
    pub text: Result<String, String>, // the briefing, or why there is none
    pub tokens_in: u64,
    pub tokens_out: u64,
    pub cache_read: u64,  // tokens Bedrock had kept from the turn before, at a tenth of the price
    pub cache_write: u64, // tokens it kept for the next
    pub dollars: f64,
    pub looks: u32,
    pub print: String, // the prompt's fingerprint, as a run records it
}

const TOOLS: &str = r#"[{"type":"function","function":{"name":"look","description":"Look at the builder's copy of the repo, as it is on its disk now, its changes included. command is list (args: a folder), read (args: a file, then optionally the first and last line to show; at most 400 lines at a time; each line comes with its number) or find (args: a word, then optionally a folder or file to look in).","parameters":{"type":"object","properties":{"command":{"type":"string","enum":["list","read","find"]},"args":{"type":"array","items":{"type":"string"}}},"required":["command","args"]}}},{"type":"function","function":{"name":"send_briefing","description":"Send the briefing and end the call. Only this reaches the builder. Every entry of where quotes lines copied exactly from the file it names; a quote that is not in the file is refused.","parameters":{"type":"object","properties":{"wrong":{"type":"string","description":"What is actually wrong, in plain words: the cause, not the symptom. A few sentences."},"where":{"type":"array","description":"The places in the repo this rests on, most important first, one to six.","items":{"type":"object","properties":{"file":{"type":"string"},"quote":{"type":"string","description":"One to eight lines copied exactly from the file, without line numbers."},"why":{"type":"string","description":"What these lines show, in one sentence."}},"required":["file","quote","why"]}},"example":{"type":"string","description":"The shape of the change, at most twenty lines; never the whole fix."},"first":{"type":"string","description":"What to do first, and what to leave for later."},"unsure":{"type":"string","description":"What you could not check, if anything."}},"required":["wrong","where","first"]}}}]"#;

struct Friend<'a> {
    root: &'a Path,
    runs: &'a Runs,
    recorded: String,
    max_usd: f64,
    looks: u32,
    briefing: Option<String>,
}

impl Friend<'_> {
    /// the briefing as the card reads it, or what is wrong with it
    fn check(&self, a: &Value) -> Result<String, String> {
        let s = |k: &str| a[k].as_str().unwrap_or("").trim().to_string();
        let wrong = s("wrong");
        if wrong.is_empty() {
            return Err("wrong is empty: say what is actually wrong".into());
        }
        let places = a["where"].as_array().cloned().unwrap_or_default();
        if places.is_empty() || places.len() > 6 {
            return Err("where needs one to six places, each a file and lines quoted from it".into());
        }
        let mut out = format!("What is actually wrong:\n{wrong}\n\nWhere to look:\n");
        for p in &places {
            let (file, quote, why) = (p["file"].as_str().unwrap_or("").trim(), p["quote"].as_str().unwrap_or(""), p["why"].as_str().unwrap_or("").trim());
            if file.is_empty() || quote.trim().is_empty() || file.split('/').any(|x| x == ".." || x.is_empty()) {
                return Err("each place is a file in the repo and a quote from it".into());
            }
            let text = fs::read_to_string(self.root.join(file)).map_err(|_| format!("there is no file {file} on the builder's disk"))?;
            // each quoted line must be there, as written but for the space around it
            let lines: Vec<&str> = quote.lines().map(str::trim).filter(|l| !l.is_empty()).collect();
            if lines.len() > 8 {
                return Err(format!("the quote from {file} is more than eight lines"));
            }
            let mut at = 0usize;
            for l in &lines {
                match text[at..].find(l) {
                    Some(i) => at += i + l.len(),
                    None => return Err(format!("{file} does not hold this line of your quote, in order: \"{}\".  Copy the lines exactly, or look again", runtime::model::cut(l, 120))),
                }
            }
            let line_no = text[..text.find(lines[0]).unwrap_or(0)].lines().count() + 1;
            out += &format!("- {file}, line {line_no}: {why}\n");
            for l in &lines {
                out += &format!("      {l}\n");
            }
        }
        let example = s("example");
        if example.lines().count() > 24 {
            return Err("example is more than twenty lines: the shape of the change, never the whole fix".into());
        }
        if !example.is_empty() {
            out += &format!("\nThe shape of the change:\n{example}\n");
        }
        let first = s("first");
        if first.is_empty() {
            return Err("first is empty: say what to do first".into());
        }
        out += &format!("\nDo first:\n{first}\n");
        let unsure = s("unsure");
        if !unsure.is_empty() {
            out += &format!("\nNot checked:\n{unsure}\n");
        }
        Ok(out)
    }
}

impl Hooks for Friend<'_> {
    fn tools(&self) -> Vec<Value> {
        serde_json::from_str(TOOLS).expect("the friend's tools are JSON")
    }

    fn before(&mut self, turn: u32, _messages: &mut Vec<Value>, tin: u64, tout: u64) -> Result<(), (String, String)> {
        // each turn is asked of the friend's share, then of the month's cap
        let spent = self.runs.cost(&self.recorded, tin, tout).unwrap_or(0.0);
        let next = crate::build::max_call();
        if spent + next > self.max_usd {
            return Err(("budget".into(), format!("before its turn {turn}: its share, ${:.2}, has no room for a call that may cost ${next:.2} after ${spent:.4}", self.max_usd)));
        }
        self.runs.room(spent + next).map_err(|why| ("budget".to_string(), format!("before its turn {turn}: {why}")))
    }

    fn call(&mut self, name: &str, args: &Value) -> Step {
        match name {
            "look" => {
                if self.looks >= LOOKS {
                    return Step::Answer(format!("refused: you have had your {LOOKS} looks.  Send the briefing with send_briefing, and say in unsure what you could not check."));
                }
                self.looks += 1;
                let cmd = args["command"].as_str().unwrap_or("");
                let a: Vec<String> = args["args"].as_array().into_iter().flatten().filter_map(|v| v.as_str()).filter(|s| !s.is_empty()).take(6).map(String::from).collect();
                let (answer, _) = look::look(self.root, cmd, &a);
                Step::Answer(runtime::room::cap(&answer, LOOK_CHARS))
            }
            "send_briefing" => match self.check(args) {
                Ok(text) => {
                    self.briefing = Some(text);
                    Step::Finish("Sent.".into(), "briefing".into(), String::new())
                }
                Err(why) => Step::Answer(format!("refused: {why}.  Nothing was sent.")),
            },
            other => Step::Answer(format!("There is no tool {other}.  The tools are look and send_briefing.")),
        }
    }

    fn after(&mut self, _turn: u32, left: u32) -> Option<String> {
        Some(format!("{} of {LOOKS} looks used; {left} turns left.  When you know the cause, send the briefing with send_briefing.", self.looks))
    }

    fn nudge(&mut self, _stop: Stop, _turn: u32, _left: u32) -> String {
        "Nothing reaches the builder until you call send_briefing.  Send the briefing now.".into()
    }
}

/// One call to the friend: model a Bedrock model id, at most max_usd to spend,
/// root the copy of the repo it may look at.  Nothing is sent unless it can be
/// costed, has room in its share and in the month's cap, and has a key.
pub fn brief(repo: &Path, runs: &Runs, model_id: &str, max_usd: f64, root: &Path, ask: &str) -> Briefing {
    let none = |why: String| Briefing { text: Err(why), tokens_in: 0, tokens_out: 0, cache_read: 0, cache_write: 0, dollars: 0.0, looks: 0, print: String::new() };
    let recorded = format!("bedrock:{model_id}");
    if runs.cost(&recorded, 0, 0).is_none() {
        return none(format!("no price for {recorded}: a model whose calls cannot be costed is not called"));
    }
    let next = crate::build::max_call();
    if next > max_usd {
        return none(format!("its share, ${max_usd:.2}, has no room for a call that may cost ${next:.2}"));
    }
    if let Err(why) = runs.room(next) {
        return none(why);
    }
    let keys = match record::keys() {
        Ok(k) => k,
        Err(why) => return none(format!("Bedrock: {why}")),
    };
    let (system, print) = match prompts::load(&repo.join("prompts"), PROMPT) {
        Ok(p) => p,
        Err(why) => return none(why),
    };
    let m = Model {
        gear: 2,
        name: model_id.to_string(),
        url: String::new(),
        num_ctx: 200_000,
        max_tokens: 4000,
        think_cap: 0,
        timeout: Duration::from_secs(600),
        cache: std::env::var("AGENT_FRIEND_CACHE").map(|v| v != "0").unwrap_or(true),
    };
    let mut f = Friend { root, runs, recorded: recorded.clone(), max_usd, looks: 0, briefing: None };
    let r = react::run(&m, Some(&keys), &system, ask, LOOKS + 6, &mut f);
    let text = match (&r.ended, f.briefing.take()) {
        (Ended::Finished(..), Some(b)) => Ok(b),
        (Ended::Unreachable { why, .. }, _) | (Ended::Error(why), _) => Err(format!("the call failed: {why}")),
        (Ended::Halted(_, why), _) => Err(why.clone()),
        _ => Err(format!("it ended after {} turns without sending a briefing", r.calls)),
    };
    let dollars = runs.cost_cached(&recorded, r.tokens_in, r.tokens_out, r.cache_read, r.cache_write).unwrap_or(0.0);
    Briefing { text, tokens_in: r.tokens_in, tokens_out: r.tokens_out, cache_read: r.cache_read, cache_write: r.cache_write, dollars, looks: f.looks, print }
}

/// The call and its answer, kept beside the run that made it as friend-N, and
/// recorded in runs/ under friend-ITEM so the month's spend and the card see it:
/// a briefing, or a call that cost something and sent none.
#[allow(clippy::too_many_arguments)]
pub fn keep(dir: &Path, runs: &Runs, item: &str, model_id: &str, try_: u64, n: u64, at: &str, trial: &str, ask: &str, got: &Briefing, seconds: u64) -> Result<PathBuf, String> {
    let asked = dir.join(format!("friend-{n}-asked.txt"));
    let said = dir.join(format!("friend-{n}.md"));
    let _ = fs::write(&asked, ask);
    let head = format!("model: bedrock:{model_id}; {} looks; {} in, {} read from the cache, {} written to it, {} out; ${:.6}", got.looks, got.tokens_in, got.cache_read, got.cache_write, got.tokens_out, got.dollars);
    match &got.text {
        Ok(t) => {
            let _ = fs::write(&said, format!("# The friend's briefing\n\n{head}\n\n{t}\n"));
        }
        Err(why) => {
            let _ = fs::write(&said, format!("# The friend sent no briefing\n\n{head}\n\n{why}\n"));
        }
    }
    if got.tokens_in + got.tokens_out + got.cache_read + got.cache_write == 0 {
        return Ok(said); // nothing was sent, nothing was spent
    }
    let sitting = std::env::var("AGENT_SITTING").ok().filter(|s| !s.is_empty() && crate::util::plain(s, "._-"));
    let (result, ended) = if got.text.is_ok() { ("briefing", "briefing") } else { ("fail", "error") };
    runs.record_cached(
        &format!("friend-{item}"), &format!("bedrock:{model_id}"), try_ * 100 + n, got.tokens_in, got.tokens_out, result, 0, &asked, &said, at, &got.print,
        sitting.as_deref(), got.tokens_in + got.tokens_out, ended, trial, (got.looks + 1) as u64, seconds, got.cache_read, got.cache_write,
    )?;
    Ok(said)
}
