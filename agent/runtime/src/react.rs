// SPDX-License-Identifier: GPL-2.0-only
//! The ReAct loop: reason, call a tool, read the answer, again.  Ported from
//! CivicCanada's AgentLoop as logic, not code (agent/origin.toml): the system
//! prompt holds the role and the rules, the first user message the one job; the
//! calls are capped (AgentBudget.MaxTurns); a call whose arguments cannot be read
//! is answered with what was wrong and the loop goes on, since small models
//! recover once told (AgentTurn.ParseError fed back as a correction); and a run
//! that ends without finishing says why (Halt).  What a tool does, what is checked
//! before each call (room, money), and what is said after each answer belong to
//! whoever uses the loop, through Hooks.

use crate::model::{self, Keys, Model, Stop};
use serde_json::{json, Value};

/// What a tool call came to.
pub enum Step {
    /// an answer, handed back to the model
    Answer(String),
    /// an answer, and the run is over: how (a word) and why
    Finish(String, String, String),
}

pub trait Hooks {
    /// the tools offered, in Ollama's shape
    fn tools(&self) -> Vec<Value>;
    /// before each call; Err(how, why) ends the run without making it
    fn before(&mut self, turn: u32, messages: &mut Vec<Value>, tokens_in: u64, tokens_out: u64) -> Result<(), (String, String)>;
    /// one tool call, its arguments read
    fn call(&mut self, name: &str, args: &Value) -> Step;
    /// after a turn's answers, a note for the model if any (calls left, say)
    fn after(&mut self, turn: u32, left: u32) -> Option<String>;
    /// when the model stops without a tool, what to say (once)
    fn nudge(&mut self, stop: Stop, turn: u32, left: u32) -> String;
    /// after each reply, the run's tokens so far, before its tools are called
    fn tally(&mut self, _tokens_in: u64, _tokens_out: u64) {}
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Ended {
    /// a tool finished it: how and why
    Finished(String, String),
    /// out of calls
    Turns,
    /// the conversation outgrew the context
    Room,
    /// stopped without a tool, twice, or cut off at its own limit
    Stopped(String),
    /// the model could not be reached; on the first call, nothing was spent
    Unreachable { first_call: bool, why: String },
    /// the model answered with an error
    Error(String),
    /// a hook said no before a call: how and why
    Halted(String, String),
}

pub struct Run {
    pub messages: Vec<Value>,
    pub calls: u32,
    pub tokens_in: u64,
    pub tokens_out: u64,
    pub peak: u64,
    pub ended: Ended,
}

/// A call's arguments: an object, or a string holding one.
pub fn arguments(call: &Value) -> Option<Value> {
    match &call["function"]["arguments"] {
        Value::Object(_) => Some(call["function"]["arguments"].clone()),
        Value::String(s) => serde_json::from_str::<Value>(s).ok().filter(Value::is_object),
        _ => None,
    }
}

pub fn run(
    m: &Model,
    keys: Option<&Keys>,
    system: &str,
    first: &str,
    max_turns: u32,
    hooks: &mut dyn Hooks,
) -> Run {
    let mut r = Run {
        messages: vec![json!({"role": "user", "content": first})],
        calls: 0,
        tokens_in: 0,
        tokens_out: 0,
        peak: 0,
        ended: Ended::Turns,
    };
    let tools = hooks.tools();
    let mut nudged = false;
    for turn in 1..=max_turns {
        if let Err((how, why)) = hooks.before(turn, &mut r.messages, r.tokens_in, r.tokens_out) {
            r.ended = Ended::Halted(how, why);
            return r;
        }
        let reply = model::chat(m, system, &r.messages, &tools, keys);
        r.calls = turn;
        r.tokens_in += reply.tokens_in;
        r.tokens_out += reply.tokens_out;
        r.peak = r.peak.max(reply.tokens_in + reply.tokens_out);
        hooks.tally(r.tokens_in, r.tokens_out);
        match reply.stop {
            Stop::Unreachable => {
                r.ended = Ended::Unreachable { first_call: turn == 1, why: reply.error };
                return r;
            }
            Stop::Error => {
                r.ended = Ended::Error(reply.error);
                return r;
            }
            _ => {}
        }
        let msg = reply.message.unwrap_or_else(|| json!({"role": "assistant", "content": ""}));
        r.messages.push(msg.clone());
        let left = max_turns - turn;
        match reply.stop {
            // nothing more can be said to it (decision 0090)
            Stop::Context => {
                r.ended = Ended::Room;
                return r;
            }
            Stop::Tool => {}
            stop => {
                if !nudged && left > 0 {
                    nudged = true;
                    let n = hooks.nudge(stop, turn, left);
                    r.messages.push(json!({"role": "user", "content": n}));
                    continue;
                }
                r.ended = Ended::Stopped(if stop == Stop::MaxTokens {
                    "its reply was cut off at its limit".into()
                } else {
                    "it stopped without a tool".into()
                });
                return r;
            }
        }
        let mut finished: Option<(String, String)> = None;
        for call in msg["tool_calls"].as_array().cloned().unwrap_or_default() {
            let name = call["function"]["name"].as_str().unwrap_or("?").to_string();
            let answer = if finished.is_some() {
                "not run: the run ended with the call before it".to_string()
            } else {
                match arguments(&call) {
                    // a call that cannot be read is answered, not fatal
                    None => format!("Your call to {name} could not be read: its arguments are not a JSON object.  Call it again with JSON arguments."),
                    Some(args) => match hooks.call(&name, &args) {
                        Step::Answer(a) => a,
                        Step::Finish(a, how, why) => {
                            finished = Some((how, why));
                            a
                        }
                    },
                }
            };
            r.messages.push(json!({"role": "tool", "tool_name": name, "content": answer}));
        }
        if let Some((how, why)) = finished {
            r.ended = Ended::Finished(how, why);
            return r;
        }
        if let Some(note) = hooks.after(turn, left) {
            r.messages.push(json!({"role": "user", "content": note}));
        }
    }
    r.ended = Ended::Turns;
    r
}

#[cfg(all(test, feature = "fake"))]
mod tests {
    use super::*;
    use std::sync::Mutex;
    use std::time::Duration;

    static ONE_AT_A_TIME: Mutex<()> = Mutex::new(());

    fn reply(msg: Value, done: &str, tin: u64, tout: u64) -> String {
        json!({"message": msg, "done": true, "done_reason": done, "prompt_eval_count": tin, "eval_count": tout}).to_string()
    }
    fn tool(name: &str, args: Value) -> Value {
        json!({"role": "assistant", "content": "", "tool_calls": [{"function": {"name": name, "arguments": args}}]})
    }

    /// saved replies in a fresh folder, handed to the fake transport
    fn fake(name: &str, replies: &[String]) -> std::path::PathBuf {
        let dir = std::env::temp_dir().join(format!("react-{name}-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(&dir).unwrap();
        for (i, r) in replies.iter().enumerate() {
            std::fs::write(dir.join(format!("{}.json", i + 1)), r).unwrap();
        }
        std::env::set_var("AGENT_FAKE", &dir);
        dir
    }

    fn model() -> Model {
        Model { gear: 1, name: "m".into(), url: "http://x".into(), num_ctx: 8192, max_tokens: 100, think_cap: 0, timeout: Duration::from_secs(5) }
    }

    struct Script {
        seen: Vec<(String, Value)>,
        halt_at: Option<u32>,
    }

    impl Hooks for Script {
        fn tools(&self) -> Vec<Value> {
            vec![json!({"type": "function", "function": {"name": "look", "description": "d", "parameters": {"type": "object"}}})]
        }
        fn before(&mut self, turn: u32, _: &mut Vec<Value>, _: u64, _: u64) -> Result<(), (String, String)> {
            if Some(turn) == self.halt_at { Err(("budget".into(), "no room".into())) } else { Ok(()) }
        }
        fn call(&mut self, name: &str, args: &Value) -> Step {
            self.seen.push((name.into(), args.clone()));
            if name == "done" { Step::Finish("ok".into(), "pass".into(), "done".into()) } else { Step::Answer(format!("answer {}", self.seen.len())) }
        }
        fn after(&mut self, turn: u32, left: u32) -> Option<String> {
            Some(format!("Call {turn} done; {left} left."))
        }
        fn nudge(&mut self, _: Stop, _: u32, _: u32) -> String {
            "You stopped.".into()
        }
    }

    #[test]
    fn a_call_it_cannot_read_is_answered_and_the_loop_goes_on() {
        let _g = ONE_AT_A_TIME.lock().unwrap();
        fake("parse", &[
            reply(json!({"role": "assistant", "content": "", "tool_calls": [{"function": {"name": "look", "arguments": "{oops"}}]}), "stop", 10, 1),
            reply(tool("look", json!({"why": "w"})), "stop", 20, 2),
            reply(tool("done", json!({})), "stop", 30, 3),
        ]);
        let mut s = Script { seen: vec![], halt_at: None };
        let r = run(&model(), None, "sys", "job", 10, &mut s);
        assert_eq!(r.ended, Ended::Finished("pass".into(), "done".into()));
        assert_eq!(r.calls, 3);
        assert_eq!((r.tokens_in, r.tokens_out, r.peak), (60, 6, 33));
        assert!(r.messages[2]["content"].as_str().unwrap().contains("could not be read"), "{}", r.messages[2]);
        assert_eq!(s.seen.len(), 2, "the unreadable call never reached the tool");
        assert_eq!(r.messages[3]["content"], "Call 1 done; 9 left.");
    }

    #[test]
    fn a_stop_is_nudged_once_then_ends() {
        let _g = ONE_AT_A_TIME.lock().unwrap();
        let said = reply(json!({"role": "assistant", "content": "I think so"}), "stop", 5, 5);
        fake("nudge", &[said.clone(), said]);
        let mut s = Script { seen: vec![], halt_at: None };
        let r = run(&model(), None, "sys", "job", 10, &mut s);
        assert_eq!(r.ended, Ended::Stopped("it stopped without a tool".into()));
        assert_eq!(r.messages[2]["content"], "You stopped.");
        assert_eq!(r.calls, 2);
    }

    #[test]
    fn the_calls_are_capped() {
        let _g = ONE_AT_A_TIME.lock().unwrap();
        let look = reply(tool("look", json!({})), "stop", 1, 1);
        fake("turns", &[look.clone(), look.clone(), look]);
        let r = run(&model(), None, "sys", "job", 2, &mut Script { seen: vec![], halt_at: None });
        assert_eq!(r.ended, Ended::Turns);
        assert_eq!(r.calls, 2);
    }

    #[test]
    fn a_full_context_is_room_and_a_cut_reply_is_not() {
        let _g = ONE_AT_A_TIME.lock().unwrap();
        fake("room", &[reply(json!({"role": "assistant", "content": ""}), "length", 8000, 40)]);
        let r = run(&model(), None, "sys", "job", 5, &mut Script { seen: vec![], halt_at: None });
        assert_eq!(r.ended, Ended::Room);
        fake("cut", &[reply(json!({"role": "assistant", "content": ""}), "length", 800, 100)]);
        let r = run(&model(), None, "sys", "job", 1, &mut Script { seen: vec![], halt_at: None });
        assert_eq!(r.ended, Ended::Stopped("its reply was cut off at its limit".into()));
    }

    #[test]
    fn nothing_there_on_the_first_call_is_said_so() {
        let _g = ONE_AT_A_TIME.lock().unwrap();
        fake("gone", &[]);
        let r = run(&model(), None, "sys", "job", 5, &mut Script { seen: vec![], halt_at: None });
        assert!(matches!(r.ended, Ended::Unreachable { first_call: true, .. }));
    }

    #[test]
    fn a_hook_can_stop_a_call_before_it_is_made() {
        let _g = ONE_AT_A_TIME.lock().unwrap();
        let dir = fake("halt", &[reply(tool("look", json!({})), "stop", 1, 1), reply(tool("look", json!({})), "stop", 1, 1)]);
        let r = run(&model(), None, "sys", "job", 5, &mut Script { seen: vec![], halt_at: Some(2) });
        assert_eq!(r.ended, Ended::Halted("budget".into(), "no room".into()));
        assert!(!dir.join("sent-2.json").exists(), "the second call was not sent");
        assert_eq!(r.calls, 1);
    }
}
