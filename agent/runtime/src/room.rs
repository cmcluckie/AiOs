// SPDX-License-Identifier: GPL-2.0-only
//! Room: keeping a conversation inside the model's context.  Ported from
//! CivicCanada's agent as logic, not code (agent/origin.toml): a tool's answer is
//! cut at a limit with a marker that says how much more there was and to narrow the
//! ask (AgentLoop.Truncate); tokens are estimated as characters over four, rough on
//! purpose (ConversationCondenser.EstimateTokens); and past its share of the
//! window, everything but the newest messages is folded and those kept word for
//! word (ConversationCondenser.Plan, KeepMessages).  CivicCanada folds into a
//! summary a model writes.  Here old tool answers are dropped to a line that says
//! so, since they can be asked for again, and nothing calls a model to do it.

use serde_json::Value;

/// The newest messages, always kept word for word (CivicCanada's KeepMessages).
pub const KEEP: usize = 4;

/// Characters over four, rounded up.
pub fn tokens(s: &str) -> u64 {
    (s.chars().count() as u64).div_ceil(4)
}

/// Cuts text at max characters, saying how many more there were.
pub fn cap(text: &str, max: usize) -> String {
    let n = text.chars().count();
    if n <= max {
        return text.to_string();
    }
    let kept: String = text.chars().take(max).collect();
    format!("{kept}\n… truncated, {} more characters. Narrow the query.", n - max)
}

fn size_of(m: &Value) -> u64 {
    let mut n = tokens(m.get("content").and_then(Value::as_str).unwrap_or(""));
    if let Some(c) = m.get("tool_calls") {
        n += tokens(&c.to_string());
    }
    n
}

/// The conversation's estimated size in tokens.
pub fn size(messages: &[Value]) -> u64 {
    messages.iter().map(size_of).sum()
}

/// Over budget, the content of each tool answer but the newest KEEP messages is
/// dropped, oldest first, until it fits; each says it was.  The characters dropped.
pub fn drop_old(messages: &mut [Value], budget: u64) -> usize {
    let mut size = size(messages);
    let mut dropped = 0;
    let last = messages.len().saturating_sub(KEEP);
    for m in messages[..last].iter_mut() {
        if size <= budget {
            break;
        }
        if m.get("role").and_then(Value::as_str) != Some("tool") {
            continue;
        }
        let content = m.get("content").and_then(Value::as_str).unwrap_or("").to_string();
        if content.starts_with("[dropped") {
            continue;
        }
        let n = content.chars().count();
        let stub = format!("[dropped to make room: this answer was {n} characters; look again if you need it]");
        size = size - tokens(&content) + tokens(&stub);
        dropped += n;
        m["content"] = Value::String(stub);
    }
    dropped
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn cap_says_how_much_was_cut() {
        assert_eq!(cap("abcdef", 10), "abcdef");
        assert_eq!(cap("abcdef", 4), "abcd\n… truncated, 2 more characters. Narrow the query.");
    }

    #[test]
    fn tokens_are_characters_over_four() {
        assert_eq!(tokens(""), 0);
        assert_eq!(tokens("abcd"), 1);
        assert_eq!(tokens("abcde"), 2);
    }

    #[test]
    fn drops_the_oldest_answers_and_keeps_the_newest_four() {
        let big = "x".repeat(4000);
        let mut m: Vec<Value> = vec![json!({"role": "user", "content": "the job"})];
        for _ in 0..4 {
            m.push(json!({"role": "assistant", "content": ""}));
            m.push(json!({"role": "tool", "content": big}));
        }
        let dropped = drop_old(&mut m, 2500);
        assert_eq!(dropped, 8000, "two answers of 4000 dropped, enough to fit");
        assert!(m[2]["content"].as_str().unwrap().starts_with("[dropped to make room: this answer was 4000 characters"));
        assert!(m[4]["content"].as_str().unwrap().starts_with("[dropped"));
        assert_eq!(m[6]["content"], big, "under budget once two are gone");
        assert_eq!(m[8]["content"], big, "the newest four are never dropped");
        assert_eq!(m[0]["content"], "the job", "only tool answers are dropped");
        let mut tight = m.clone();
        drop_old(&mut tight, 0);
        assert_eq!(tight[8]["content"], big, "not even when nothing else is left to drop");
        assert_eq!(tight[6]["content"], big, "message 6 of 9 is among the newest four");
    }
}
