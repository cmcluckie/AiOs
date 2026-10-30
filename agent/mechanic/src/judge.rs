// SPDX-License-Identifier: GPL-2.0-only
//! The judge: a builder's fix run against the suite on the jig, over SSH.  Nothing
//! new is installed there: one command, sent with ssh, unpacks the repo at the
//! job's commit into a throwaway folder from the jig's own clone, lays the fix's
//! files over it from a tar on its input, runs jig/test, and is gone (findings
//! 0027).  What a fix may touch and when it is a pass are jig/fix's rules
//! (decisions 0060 and 0068), with two holes closed: a test green before must be
//! green after, not merely not red, and the red test must print PASS and no FAIL.

use crate::util::plain;
use std::collections::BTreeMap;
use std::path::Path;
use std::process::{Command, Stdio};

pub struct Jig {
    pub ssh: String,  // the ssh to run, AGENT_SSH; "ssh"
    pub host: String, // AGENT_JIG; chris@172.19.238.112
    pub repo: String, // the jig's clone under its home, AGENT_JIG_REPO; aios
}

impl Jig {
    pub fn from_env() -> Jig {
        let get = |k: &str, d: &str| std::env::var(k).ok().filter(|v| !v.is_empty()).unwrap_or_else(|| d.to_string());
        Jig { ssh: get("AGENT_SSH", "ssh"), host: get("AGENT_JIG", "chris@172.19.238.112"), repo: get("AGENT_JIG_REPO", "aios") }
    }

    /// Runs the suite on the jig at commit, with the files under fixed (None: the
    /// repo as it was) laid over it.  What jig/test printed, or why it did not run.
    pub fn run(&self, at: &str, suite: &str, fixed: Option<&Path>) -> Result<String, String> {
        if !plain(at, "") || at.is_empty() || !plain(suite, " .@_-") || !plain(&self.repo, "._-/") || !plain(&self.host, "@._-") {
            return Err("the commit, the suite, the jig or its clone has a character a command may not".into());
        }
        // the remote half, one word to ssh; every value in it checked above
        let r = &self.repo;
        let remote = format!(
            "d=$(mktemp -d) && trap \"rm -rf $d\" EXIT && \
             git -C \"$HOME/{r}\" archive {at} | tar -xf - -C \"$d\" && rm -rf \"$d/runs\" \"$d/out\" && \
             tar -xf - -C \"$d\" && cd \"$d\" && \
             LINUX=\"$HOME/{r}/../linux\" MUSL=\"$HOME/{r}/../musl\" V7=\"$HOME/{r}/../unix-history-repo\" \
             BSD=\"$HOME/{r}/../unix-history-repo\" sh jig/test {suite} 2>&1"
        );
        let mut tar = Command::new("tar");
        match fixed {
            Some(dir) => tar.arg("-cf").arg("-").arg("-C").arg(dir).arg("."),
            None => tar.args(["-cf", "-", "-T", "/dev/null"]),
        };
        let tarred = tar.output().map_err(|e| format!("tar: {e}"))?;
        // AGENT_SSH may be a command with words of its own ("sh fake-ssh"), never more
        let words: Vec<&str> = self.ssh.split_whitespace().collect();
        if words.is_empty() || !words.iter().all(|w| plain(w, "/._-=")) {
            return Err("AGENT_SSH is not a command".into());
        }
        let mut child = Command::new(words[0])
            .args(&words[1..])
            .arg(&self.host)
            .arg(&remote)
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
            .map_err(|e| format!("{}: {e}", self.ssh))?;
        {
            use std::io::Write;
            let mut stdin = child.stdin.take().unwrap();
            let _ = stdin.write_all(&tarred.stdout);
        }
        let out = child.wait_with_output().map_err(|e| e.to_string())?;
        let text = String::from_utf8_lossy(&out.stdout).to_string();
        if text.lines().any(|l| l.starts_with("# ") && l.contains(" passed, ")) {
            Ok(text)
        } else {
            let err = String::from_utf8_lossy(&out.stderr);
            Err(format!("the suite did not run on {}: {}", self.host, err.lines().next().unwrap_or(text.lines().next().unwrap_or(""))))
        }
    }
}

impl Jig {
    /// One question to the shared source reader on the jig (jig/reader), as jig/sit
    /// asks it: the same pinned Linux, the same answers.  What it printed, and
    /// whether it answered; or why it could not be asked.
    pub fn reader(&self, command: &str, args: &[String]) -> Result<(String, bool), String> {
        const COMMANDS: [&str; 7] = ["list", "read", "find", "callers", "calls", "ports", "check"];
        if !COMMANDS.contains(&command) {
            return Err(format!("the reader has no command '{command}'; its commands are {}", COMMANDS.join(", ")));
        }
        if args.len() > 4 || args.iter().any(|a| a.is_empty() || a.len() > 200 || !plain(a, "._-/+,") || a.starts_with('-')) {
            return Err("each argument is a path, a name or a line number: letters, digits and . _ - / + , only, at most four".into());
        }
        if !plain(&self.repo, "._-/") || !plain(&self.host, "@._-") {
            return Err("the jig or its clone has a character a command may not".into());
        }
        let r = &self.repo;
        // every value in it checked above, so it is safe as one word for ssh
        let remote = format!("cd \"$HOME/{r}\" && LINUX=\"$HOME/{r}/../linux\" sh jig/reader {command} {}", args.join(" "));
        let words: Vec<&str> = self.ssh.split_whitespace().collect();
        if words.is_empty() || !words.iter().all(|w| plain(w, "/._-=")) {
            return Err("AGENT_SSH is not a command".into());
        }
        let out = Command::new(words[0])
            .args(&words[1..])
            .arg(&self.host)
            .arg(&remote)
            .stdin(Stdio::null())
            .output()
            .map_err(|e| format!("{}: {e}", self.ssh))?;
        if out.status.code() == Some(255) {
            return Err(format!("the jig did not answer: {}", String::from_utf8_lossy(&out.stderr).lines().next().unwrap_or("")));
        }
        let mut text = String::from_utf8_lossy(&out.stdout).to_string();
        text += &String::from_utf8_lossy(&out.stderr);
        Ok((text, out.status.success()))
    }
}

/// A lift passes when every one of the item's tests prints PASS and none prints
/// FAIL, with the lift in the hand lift's place: jig/judge's rule (exam/items.toml).
/// A test that printed nothing, or SKIP, is not a pass.  The first one that is not,
/// or None.
pub fn lift_verdict(after: &str, codes: &[String]) -> Option<String> {
    let a = results(after);
    for c in codes {
        if has(&a, c, "FAIL") {
            let l = a[c].iter().find(|(w, _)| w == "FAIL").map(|(_, l)| l.clone()).unwrap_or_default();
            return Some(runtime::model::cut(&l, 200));
        }
        if !has(&a, c, "PASS") {
            return Some(format!("{c} did not run, or was skipped"));
        }
    }
    None
}

/// A fix is the files of one part, each an existing file under pool/ in base, every
/// part of its path a name: no "..", no ".", nothing empty.  Checked before
/// anything is written, so a path the model chose cannot put a file anywhere else.
pub fn check(base: &Path, paths: &[String]) -> Result<(), String> {
    if paths.is_empty() {
        return Err("a fix is at least one file".into());
    }
    let mut part: Option<&str> = None;
    for p in paths {
        let parts: Vec<&str> = p.split('/').collect();
        let named = parts.iter().all(|s| !s.is_empty() && *s != "." && *s != "..");
        let is_file = base.join(p).symlink_metadata().map(|m| m.is_file()).unwrap_or(false);
        if parts.len() < 3 || parts[0] != "pool" || !named || !is_file || !plain(p, "/._-@") {
            return Err(format!("only a file under pool/ that already exists may be fixed, not {p}"));
        }
        match part {
            None => part = Some(parts[1]),
            Some(q) if q != parts[1] => return Err(format!("a fix is the files of one part; {p} is not in pool/{q}")),
            _ => {}
        }
    }
    Ok(())
}

/// Each test's lines: PASS, FAIL, SKIP, by code; a code may have more than one.
fn results(out: &str) -> BTreeMap<String, Vec<(String, String)>> {
    let mut m: BTreeMap<String, Vec<(String, String)>> = BTreeMap::new();
    for line in out.lines() {
        let line = line.trim_start_matches(' ');
        for word in ["PASS", "FAIL", "SKIP"] {
            if let Some(rest) = line.strip_prefix(word).and_then(|r| r.strip_prefix("  ")) {
                let code = rest.split("  ").next().unwrap_or("").to_string();
                m.entry(code).or_default().push((word.to_string(), line.to_string()));
            }
        }
    }
    m
}

fn has(m: &BTreeMap<String, Vec<(String, String)>>, code: &str, word: &str) -> bool {
    m.get(code).map(|v| v.iter().any(|(w, _)| w == word)).unwrap_or(false)
}

pub struct Verdict {
    pub pass: bool,
    pub line: String,          // one line saying why
    pub newly_red: Vec<String>, // the lines that count against the fix
}

/// A pass: the red test prints PASS and no FAIL, and every test that printed PASS
/// before prints PASS and no FAIL now.  A test green before and now red, skipped or
/// not run counts against the fix; one red before does not.
pub fn verdict(before: &str, after: &str, code: &str) -> Verdict {
    let (b, a) = (results(before), results(after));
    let green = has(&a, code, "PASS") && !has(&a, code, "FAIL");
    let mut newly = Vec::new();
    for (c, lines) in &a {
        if !has(&b, c, "FAIL") {
            for (w, l) in lines {
                if w == "FAIL" {
                    newly.push(l.clone());
                }
            }
        }
    }
    for c in b.keys() {
        if has(&b, c, "PASS") && !has(&a, c, "FAIL") && !has(&a, c, "PASS") {
            newly.push(format!("GONE  {c}  green before, now skipped or not run"));
        }
    }
    if green && newly.is_empty() {
        return Verdict { pass: true, line: "the red test passed and nothing that was green went red".into(), newly_red: newly };
    }
    let shown: Vec<String> = newly.iter().take(2).map(|l| runtime::model::cut(l, 160)).collect();
    let line = format!(
        "{}{} newly red{}{}",
        if green { "" } else { "the test it was given is still red; " },
        newly.len(),
        if newly.is_empty() { "" } else { ": " },
        shown.join(" ")
    );
    Verdict { pass: false, line, newly_red: newly }
}
