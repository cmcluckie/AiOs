// SPDX-License-Identifier: GPL-2.0-only
//! One call to a model.  Gear 1 is the card, through Ollama's chat API; gear 2 is
//! Sonnet on Bedrock's Converse API.  The conversation is kept in Ollama's shape
//! (role, content, tool_calls, tool_name), with the role and rules in the system
//! prompt and the one job in the first user message: the split CivicCanada's
//! agent makes.  Bedrock gets it in Converse's shape and hands it back in this one,
//! so nothing above this module knows which gear it is on.

use crate::transport::{self, Failure, Signing};
use serde_json::{json, Map, Value};
use std::time::Duration;

#[derive(Clone, Debug)]
pub struct Model {
    pub gear: u8,          // 1 the card, 2 Sonnet on Bedrock
    pub name: String,      // Ollama's name, or Bedrock's model id
    pub url: String,       // Ollama's address (gear 1)
    pub num_ctx: u64,      // the context the model holds
    pub max_tokens: u64,   // each reply's limit for what it says and calls
    pub think_cap: u64,    // the thinking allowed before it, apart; 0 for no cap
    pub timeout: Duration, // to wait for one reply
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Stop {
    Tool,        // asked for a tool
    End,         // said something and stopped
    MaxTokens,   // cut off at its own limit
    Context,     // the conversation filled the context
    Unreachable, // could not be reached
    Error,       // answered with an error, or with something unreadable
}

impl Stop {
    pub fn word(self) -> &'static str {
        match self {
            Stop::Tool => "tool",
            Stop::End => "end",
            Stop::MaxTokens => "max_tokens",
            Stop::Context => "context",
            Stop::Unreachable => "unreachable",
            Stop::Error => "error",
        }
    }
}

#[derive(Debug)]
pub struct Reply {
    pub stop: Stop,
    pub message: Option<Value>, // the assistant's message, in the conversation's shape
    pub tokens_in: u64,
    pub tokens_out: u64,
    pub error: String,
    /// its thinking passed think_cap, so it was stopped and asked again without
    pub thought_cut: bool,
}

impl Reply {
    fn failed(stop: Stop, error: String) -> Reply {
        Reply { stop, message: None, tokens_in: 0, tokens_out: 0, error, thought_cut: false }
    }
}

/// AWS keys for Bedrock, read by the caller from wherever it keeps them.
pub struct Keys {
    pub access_key: String,
    pub secret_key: String,
    pub session_token: Option<String>,
    pub region: String,
}

/// One call: the system prompt, the conversation so far and the tools.
pub fn chat(m: &Model, system: &str, messages: &[Value], tools: &[Value], keys: Option<&Keys>) -> Reply {
    if m.gear == 2 {
        match keys {
            Some(k) => bedrock(m, system, messages, tools, k),
            None => Reply::failed(Stop::Unreachable, "Bedrock: no key".into()),
        }
    } else {
        ollama(m, system, messages, tools)
    }
}

fn number(v: &Value, key: &str) -> u64 {
    v.get(key).and_then(Value::as_u64).unwrap_or(0)
}

/// What one reply may produce in all: its thinking, then what it says and calls.
pub fn reply_limit(m: &Model) -> u64 {
    m.max_tokens + m.think_cap
}

/// The request Ollama gets (decisions 0056, 0086 and 0090), streamed so the
/// thinking can be watched as it comes; with think false, a reply with none.
pub fn ollama_request(m: &Model, system: &str, messages: &[Value], tools: &[Value], think: bool) -> Value {
    let mut all = vec![json!({"role": "system", "content": system})];
    all.extend(messages.iter().cloned());
    let mut req = json!({
        "model": m.name, "stream": true, "keep_alive": "10m",
        "options": {"num_ctx": m.num_ctx, "num_predict": if think { reply_limit(m) } else { m.max_tokens }},
        "messages": all,
    });
    if !think {
        req["think"] = json!(false);
    }
    if !tools.is_empty() {
        req["tools"] = Value::Array(tools.to_vec());
    }
    req
}

/// A reply read whole: Ollama's final object, or one built from its stream.
pub fn ollama_reply(m: &Model, text: &str) -> Reply {
    read_reply(text, reply_limit(m))
}

/// limit: the num_predict the request was sent with
fn read_reply(text: &str, limit: u64) -> Reply {
    let got: Value = match serde_json::from_str(text) {
        Ok(v) => v,
        Err(_) => return Reply::failed(Stop::Error, format!("Ollama's reply could not be read: {}", cut(text, 200))),
    };
    let Some(Value::Object(msg)) = got.get("message") else {
        return Reply::failed(Stop::Error, format!("Ollama's reply could not be read: {}", cut(text, 200)));
    };
    let mut msg = msg.clone();
    // the thinking is counted in the tokens and not kept (decision 0086)
    msg.remove("thinking");
    let (tin, tout) = (number(&got, "prompt_eval_count"), number(&got, "eval_count"));
    let calls = msg.get("tool_calls").and_then(Value::as_array).map(|a| !a.is_empty()).unwrap_or(false);
    let stop = if calls {
        Stop::Tool
    } else if got.get("done_reason").and_then(Value::as_str) == Some("length") {
        // short of its own limit, the conversation filled the context (decision 0090)
        if tout >= limit { Stop::MaxTokens } else { Stop::Context }
    } else {
        Stop::End
    };
    Reply { stop, message: Some(Value::Object(msg)), tokens_in: tin, tokens_out: tout, error: String::new(), thought_cut: false }
}

enum Streamed {
    Whole(String),          // the reply, put back together as one object
    ThoughtTooLong(u64),    // stopped: its thinking passed the cap, about this many tokens
    Failed(Reply),
}

/// One streamed call.  Each line is a piece of the reply; the pieces are joined
/// into the one object a whole reply would have been.  With a cap, the reading
/// stops once the thinking passes it.
fn stream(m: &Model, body: &str, cap: u64) -> Streamed {
    let (mut content, mut thought, mut thinking) = (String::new(), String::new(), 0u64);
    let mut calls: Vec<Value> = Vec::new();
    let mut last = Value::Null;
    let mut bad: Option<String> = None;
    let mut over = false;
    let mut on = |line: &str| -> bool {
        let Ok(v) = serde_json::from_str::<Value>(line) else {
            bad = Some(cut(line, 200));
            return false;
        };
        if let Some(t) = v["message"]["content"].as_str() {
            content += t;
        }
        if let Some(t) = v["message"]["thinking"].as_str() {
            thinking += crate::room::tokens(t);
            thought += t;
        }
        if let Some(c) = v["message"]["tool_calls"].as_array() {
            calls.extend(c.iter().cloned());
        }
        if cap > 0 && thinking > cap && v["done"] != true {
            over = true;
            return false;
        }
        last = v;
        true
    };
    let status = match transport::post_lines(&format!("{}/api/chat", m.url), body, m.timeout, &mut on) {
        Err(Failure::Unreachable(_)) => return Streamed::Failed(Reply::failed(Stop::Unreachable, format!("could not reach Ollama at {}", m.url))),
        Err(Failure::Other(e)) => return Streamed::Failed(Reply::failed(Stop::Error, format!("Ollama: {e}"))),
        Ok(s) => s,
    };
    if over {
        return Streamed::ThoughtTooLong(thinking);
    }
    if status != 200 {
        return Streamed::Failed(Reply::failed(Stop::Error, format!("Ollama said {status}: {}", bad.unwrap_or_else(|| cut(&last.to_string(), 200)))));
    }
    if bad.is_some() || last.is_null() {
        return Streamed::Failed(Reply::failed(Stop::Error, format!("Ollama's reply could not be read: {}", bad.unwrap_or_default())));
    }
    // as a whole reply has it; ollama_reply counts the thinking and drops it
    let mut msg = json!({"role": "assistant", "content": content, "thinking": thought});
    if !calls.is_empty() {
        msg["tool_calls"] = Value::Array(calls);
    }
    last["message"] = msg;
    Streamed::Whole(last.to_string())
}

fn ollama(m: &Model, system: &str, messages: &[Value], tools: &[Value]) -> Reply {
    let body = ollama_request(m, system, messages, tools, true).to_string();
    match stream(m, &body, m.think_cap) {
        Streamed::Whole(text) => ollama_reply(m, &text),
        Streamed::Failed(r) => r,
        // thinking past its cap: stopped there, and the same turn asked again
        // with no thinking, so the reply's own limit goes to what it says and does
        Streamed::ThoughtTooLong(thought) => {
            let body = ollama_request(m, system, messages, tools, false).to_string();
            let mut r = match stream(m, &body, 0) {
                Streamed::Whole(text) => read_reply(&text, m.max_tokens),
                Streamed::Failed(r) => r,
                Streamed::ThoughtTooLong(_) => Reply::failed(Stop::Error, "Ollama thought with thinking off".into()),
            };
            r.tokens_out += thought;
            r.thought_cut = true;
            r
        }
    }
}

/// How many tokens the model counts for text, as a user message: its
/// prompt_eval_count with the text, less with nothing.  For calibrating the
/// estimate (room.rs) against the model itself.
pub fn count(m: &Model, text: &str) -> Result<u64, String> {
    let one = |t: &str| -> Result<u64, String> {
        let mut quick = m.clone();
        quick.max_tokens = 1;
        let body = ollama_request(&quick, "", &[json!({"role": "user", "content": t})], &[], false).to_string();
        match stream(&quick, &body, 0) {
            Streamed::Whole(text) => {
                let v: Value = serde_json::from_str(&text).map_err(|e| e.to_string())?;
                Ok(number(&v, "prompt_eval_count"))
            }
            Streamed::Failed(r) => Err(r.error),
            Streamed::ThoughtTooLong(_) => Err("thought with thinking off".into()),
        }
    };
    let (with, without) = (one(text)?, one("")?);
    Ok(with.saturating_sub(without))
}

pub fn cut(s: &str, n: usize) -> String {
    s.chars().take(n).collect()
}

/// The conversation in Converse's shape.  Ollama gives a tool call no id, so each
/// gets one from its place (t-MESSAGE-CALL), and the answers after it are matched
/// to the calls in order, as Ollama matches them.  Turns of one role are joined,
/// since Converse wants user and assistant in turn.
pub fn bedrock_request(system: &str, messages: &[Value], tools: &[Value], max_tokens: u64) -> Value {
    let mut out: Vec<Value> = Vec::new();
    let mut ids: Vec<String> = Vec::new();
    let mut used = 0;
    fn user_turn(out: &mut Vec<Value>) -> &mut Vec<Value> {
        let last_is_user = out.last().map(|m| m["role"] == "user").unwrap_or(false);
        if !last_is_user {
            out.push(json!({"role": "user", "content": []}));
        }
        out.last_mut().unwrap()["content"].as_array_mut().unwrap()
    }
    for (i, m) in messages.iter().enumerate() {
        let text = m.get("content").and_then(Value::as_str).unwrap_or("");
        match m.get("role").and_then(Value::as_str) {
            Some("assistant") => {
                let mut content = Vec::new();
                if !text.is_empty() {
                    content.push(json!({"text": text}));
                }
                ids.clear();
                used = 0;
                for (k, call) in m.get("tool_calls").and_then(Value::as_array).into_iter().flatten().enumerate() {
                    let id = format!("t-{}-{}", i + 1, k + 1);
                    let f = &call["function"];
                    let input = if f["arguments"].is_object() { f["arguments"].clone() } else { json!({}) };
                    content.push(json!({"toolUse": {"toolUseId": id, "name": f["name"].as_str().unwrap_or("?"), "input": input}}));
                    ids.push(id);
                }
                if content.is_empty() {
                    // Converse refuses an empty turn; an empty reply is kept as one that says so
                    content.push(json!({"text": "(the reply was empty)"}));
                }
                out.push(json!({"role": "assistant", "content": content}));
            }
            Some("tool") if used < ids.len() => {
                let id = ids[used].clone();
                used += 1;
                let t = if text.is_empty() { "(empty)" } else { text };
                user_turn(&mut out).push(json!({"toolResult": {"toolUseId": id, "content": [{"text": t}]}}));
            }
            Some("user") | Some("tool") => {
                let t = if text.is_empty() { "(empty)" } else { text };
                user_turn(&mut out).push(json!({"text": t}));
            }
            _ => {}
        }
    }
    let mut req = json!({
        "system": [{"text": system}],
        "messages": out,
        "inferenceConfig": {"maxTokens": max_tokens},
    });
    if !tools.is_empty() {
        let specs: Vec<Value> = tools
            .iter()
            .map(|t| {
                let f = &t["function"];
                json!({"toolSpec": {"name": f["name"], "description": f["description"], "inputSchema": {"json": f["parameters"]}}})
            })
            .collect();
        req["toolConfig"] = json!({"tools": specs});
    }
    req
}

pub fn bedrock_reply(text: &str) -> Reply {
    let got: Value = serde_json::from_str(text).unwrap_or(Value::Null);
    let (Some(content), Some(stop)) =
        (got["output"]["message"]["content"].as_array(), got["stopReason"].as_str())
    else {
        return Reply::failed(Stop::Error, format!("Bedrock's reply could not be read: {}", cut(text, 200)));
    };
    let mut words: Vec<&str> = Vec::new();
    let mut calls: Vec<Value> = Vec::new();
    for part in content {
        if let Some(t) = part["text"].as_str() {
            words.push(t);
        }
        if let Some(u) = part.get("toolUse") {
            calls.push(json!({"function": {"name": u["name"], "arguments": u["input"]}}));
        }
    }
    let mut msg = Map::new();
    msg.insert("role".into(), json!("assistant"));
    msg.insert("content".into(), json!(words.join("\n")));
    let has_calls = !calls.is_empty();
    if has_calls {
        msg.insert("tool_calls".into(), Value::Array(calls));
    }
    let stop = match stop {
        "tool_use" if has_calls => Stop::Tool,
        "max_tokens" => Stop::MaxTokens,
        "model_context_window_exceeded" => Stop::Context,
        _ => Stop::End,
    };
    Reply {
        stop,
        message: Some(Value::Object(msg)),
        tokens_in: number(&got["usage"], "inputTokens"),
        tokens_out: number(&got["usage"], "outputTokens"),
        error: String::new(),
        thought_cut: false,
    }
}

fn bedrock(m: &Model, system: &str, messages: &[Value], tools: &[Value], k: &Keys) -> Reply {
    let ok = |s: &str, also: &str| !s.is_empty() && s.chars().all(|c| c.is_ascii_alphanumeric() || also.contains(c));
    if !ok(&m.name, "._-:") || !ok(&k.region, "-") {
        return Reply::failed(Stop::Error, format!("Bedrock: {} in {} is not a model and a region", m.name, k.region));
    }
    let body = bedrock_request(system, messages, tools, m.max_tokens).to_string();
    let url = format!("https://bedrock-runtime.{}.amazonaws.com/model/{}/converse", k.region, m.name);
    let sign = Signing {
        region: &k.region,
        service: "bedrock",
        access_key: &k.access_key,
        secret_key: &k.secret_key,
        session_token: k.session_token.as_deref(),
    };
    match transport::post(&url, &body, Some(&sign), m.timeout) {
        Err(Failure::Unreachable(_)) => Reply::failed(Stop::Unreachable, format!("could not reach Bedrock in {}", k.region)),
        Err(Failure::Other(e)) => Reply::failed(Stop::Error, format!("Bedrock: {e}")),
        Ok((200, text)) => bedrock_reply(&text),
        // a 400 that says the input is too long is room, not an error; the wording is
        // Bedrock's as far as is known here, not yet seen from this account
        Ok((400, text)) if text.contains("too long") && text.contains("nput") => {
            Reply::failed(Stop::Context, format!("Bedrock: {}", cut(&text, 200)))
        }
        Ok((code, text)) => Reply::failed(Stop::Error, format!("Bedrock said {code}: {}", cut(&text, 200))),
    }
}
