// SPDX-License-Identifier: GPL-2.0-only
//! Call a friend (findings 0052): when the card is out of its depth, Claude, on
//! Bedrock, reads what the card was given, what it has done and what the tests
//! said, and sends back a briefing: what is actually wrong, where in the repo to
//! look, and a few lines of example.  It writes no fix and calls no tools; the
//! card still does the work.  One call, asked of its share and the month's cap
//! first, recorded in runs/ as a briefing, so the month's spend counts it.

use crate::record::{self, Runs};
use runtime::model::{self, Model, Stop};
use runtime::prompts;
use serde_json::json;
use std::fs;
use std::path::Path;
use std::time::Duration;

pub const PROMPT: &str = "friend@1";

pub struct Briefing {
    pub text: String,
    pub tokens_in: u64,
    pub tokens_out: u64,
    pub dollars: f64,
    pub print: String, // the prompt's fingerprint, as a run records it
}

/// One call to the friend, model a Bedrock model id, with at most max_usd to
/// spend; the briefing, or why there is none.  Nothing is sent unless it can be
/// costed, has room in its share and in the month's cap, and has a key.
pub fn brief(repo: &Path, runs: &Runs, model_id: &str, max_usd: f64, ask: &str) -> Result<Briefing, String> {
    let recorded = format!("bedrock:{model_id}");
    if runs.cost(&recorded, 0, 0).is_none() {
        return Err(format!("no price for {recorded}: a model whose calls cannot be costed is not called"));
    }
    let next = crate::build::max_call();
    if next > max_usd {
        return Err(format!("its share, ${max_usd:.2}, has no room for a call that may cost ${next:.2}"));
    }
    runs.room(next)?;
    let keys = record::keys().map_err(|why| format!("Bedrock: {why}"))?;
    let (system, print) = prompts::load(&repo.join("prompts"), PROMPT)?;
    let m = Model {
        gear: 2,
        name: model_id.to_string(),
        url: String::new(),
        num_ctx: 200_000,
        max_tokens: 4000,
        think_cap: 0,
        timeout: Duration::from_secs(600),
    };
    let reply = model::chat(&m, &system, &[json!({"role": "user", "content": ask})], &[], Some(&keys));
    if matches!(reply.stop, Stop::Unreachable | Stop::Error) {
        return Err(format!("the call failed: {}", reply.error));
    }
    let text = reply.message.as_ref().and_then(|v| v["content"].as_str()).unwrap_or("").trim().to_string();
    if text.is_empty() {
        return Err("it answered with nothing".into());
    }
    let dollars = runs.cost(&recorded, reply.tokens_in, reply.tokens_out).unwrap_or(0.0);
    Ok(Briefing { text, tokens_in: reply.tokens_in, tokens_out: reply.tokens_out, dollars, print })
}

/// The call and its answer, kept beside the run that made it, and recorded in
/// runs/ under friend-ITEM so the month's spend and the card see it.
#[allow(clippy::too_many_arguments)]
pub fn keep(dir: &Path, runs: &Runs, item: &str, model_id: &str, try_: u64, at: &str, trial: &str, ask: &str, got: &Result<Briefing, String>, seconds: u64) -> Result<(), String> {
    let asked = dir.join("friend-asked.txt");
    let said = dir.join("friend.md");
    let _ = fs::write(&asked, ask);
    match got {
        Ok(b) => {
            let _ = fs::write(&said, format!("# The friend's briefing\n\nmodel: bedrock:{model_id}; {} in, {} out; ${:.6}\n\n{}\n", b.tokens_in, b.tokens_out, b.dollars, b.text));
            let sitting = std::env::var("AGENT_SITTING").ok().filter(|s| !s.is_empty() && crate::util::plain(s, "._-"));
            runs.record(
                &format!("friend-{item}"), &format!("bedrock:{model_id}"), try_, b.tokens_in, b.tokens_out, "briefing", 0, &asked, &said, at, &b.print,
                sitting.as_deref(), b.tokens_in + b.tokens_out, "briefing", trial, 1, seconds,
            ).map(|_| ())
        }
        Err(why) => {
            let _ = fs::write(&said, format!("# The friend was not called\n\n{why}\n"));
            Ok(())
        }
    }
}
