// SPDX-License-Identifier: GPL-2.0-only
//! agent: the layered mechanic (findings 0027).  One command per piece, so each
//! piece can be run and tested on its own.
//!
//!   agent note JOB TRY TOKENS SECONDS [FILE...] < test-output
//!       the result note for one builder's run (SPEC.agent.1)
//!   agent plan FILE
//!       a part plan read and written back, every default filled in (SPEC.agent.2)
//!   agent chat MODEL NUM_CTX MAX_TOKENS < conversation.json
//!       one call to the card through Ollama at OLLAMA_URL; the conversation is
//!       {"system", "messages", "tools"}; prints how it stopped, the tokens and the
//!       reply (SPEC.agent.3); AGENT_GEAR=2 makes it one call to Sonnet on Bedrock,
//!       MODEL its id (SPEC.agent.6)
//!   agent build JOB
//!       one builder on the job in the file JOB, its fix judged on the jig over ssh
//!       (SPEC.agent.4)
//!   agent direct PLAN
//!       one director on the part plan in the file PLAN (SPEC.agent.5)
//!
//! Exit 0 done, 1 failed or not a pass, 2 asked wrongly or cannot run here.

mod build;
mod direct;
mod judge;
mod look;
mod note;
mod plan;
mod record;
mod stuck;
mod util;

use runtime::model::{self, Model};
use serde_json::Value;
use std::io::Read;
use std::time::Duration;

fn number(what: &str, s: &str) -> Result<u64, i32> {
    if s.is_empty() || !s.chars().all(|c| c.is_ascii_digit()) {
        eprintln!("agent: {what} is a whole number, not '{s}'");
        return Err(2);
    }
    s.parse().map_err(|_| {
        eprintln!("agent: {what} is a whole number, not '{s}'");
        2
    })
}

fn cmd_note(a: &[String]) -> i32 {
    if a.len() < 4 {
        eprintln!("agent: note needs JOB TRY TOKENS SECONDS [FILE...]");
        return 2;
    }
    let (t, k, s) = match (number("try", &a[1]), number("tokens", &a[2]), number("seconds", &a[3])) {
        (Ok(t), Ok(k), Ok(s)) => (t, k, s),
        _ => return 2,
    };
    let mut input = Vec::new();
    if std::io::stdin().read_to_end(&mut input).is_err() {
        eprintln!("agent: the test output could not be read");
        return 1;
    }
    let files: Vec<String> = a[4..].to_vec();
    print!("{}", note::write(&String::from_utf8_lossy(&input), &note::Job { job: &a[0], try_: t, tokens: k, seconds: s, files: &files }));
    0
}

fn cmd_plan(a: &[String]) -> i32 {
    if a.len() != 1 {
        eprintln!("agent: plan needs FILE");
        return 2;
    }
    match plan::read(&a[0]) {
        Ok(p) => {
            print!("{}", plan::write(&p));
            0
        }
        Err(why) => {
            eprintln!("agent: plan {}: {why}", a[0]);
            1
        }
    }
}

fn cmd_chat(a: &[String]) -> i32 {
    if a.len() != 3 {
        eprintln!("agent: chat needs MODEL NUM_CTX MAX_TOKENS");
        return 2;
    }
    let (ctx, max) = match (number("num_ctx", &a[1]), number("max_tokens", &a[2])) {
        (Ok(c), Ok(m)) => (c, m),
        _ => return 2,
    };
    let gear = if std::env::var("AGENT_GEAR").as_deref() == Ok("2") { 2 } else { 1 };
    let mut text = String::new();
    let _ = std::io::stdin().read_to_string(&mut text);
    let conv: Value = serde_json::from_str(&text).unwrap_or(Value::Null);
    let (Some(system), Some(messages)) = (conv["system"].as_str(), conv["messages"].as_array()) else {
        eprintln!("agent: chat needs {{\"system\": \"...\", \"messages\": [...]}} on its input");
        return 2;
    };
    let tools: Vec<Value> = conv["tools"].as_array().cloned().unwrap_or_default();
    let keys = if gear == 2 {
        match record::keys() {
            Ok(k) => Some(k),
            Err(why) => {
                println!("stop       = \"unreachable\"\ntokens_in  = 0\ntokens_out = 0");
                eprintln!("agent: Bedrock: {why}");
                return 1;
            }
        }
    } else {
        None
    };
    let m = Model {
        gear,
        name: a[0].clone(),
        url: std::env::var("OLLAMA_URL").ok().filter(|v| !v.is_empty()).unwrap_or_else(|| "http://localhost:11434".into()),
        num_ctx: ctx,
        max_tokens: max,
        timeout: Duration::from_secs(1800),
    };
    let r = model::chat(&m, system, messages, &tools, keys.as_ref());
    println!("stop       = \"{}\"\ntokens_in  = {}\ntokens_out = {}", r.stop.word(), r.tokens_in, r.tokens_out);
    if let Some(msg) = &r.message {
        println!("message    = {msg}");
    }
    if matches!(r.stop, model::Stop::Unreachable | model::Stop::Error) {
        eprintln!("agent: {}", r.error);
        return 1;
    }
    0
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let rest = if args.len() > 2 { &args[2..] } else { &[][..] };
    let rc = match args.get(1).map(String::as_str) {
        Some("note") => cmd_note(rest),
        Some("plan") => cmd_plan(rest),
        Some("chat") => cmd_chat(rest),
        Some("build") if rest.len() == 1 => build::run(&rest[0]),
        Some("direct") if rest.len() == 1 => direct::run(&rest[0]),
        _ => {
            eprintln!("agent: needs a command: note, plan, chat, build or direct");
            2
        }
    };
    std::process::exit(rc);
}
