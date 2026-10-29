// SPDX-License-Identifier: GPL-2.0-only
//! The result note, from jig/test's output (SPEC.agent.1).  Every test prints one
//! line that starts PASS, FAIL or SKIP, two spaces, its code, two spaces, its time,
//! two spaces, its message.  The counts come from those lines, not from the summary
//! at the end, so a note never says more than the lines it read.  New for AiOs:
//! CivicCanada's agent has no director, so nothing here is ported.

pub const MAX_NAMES: usize = 20; // failing tests named; the rest are counted
pub const MAX_FILES: usize = 20; // files named; the rest are counted
pub const LINE: usize = 160; // the first failure, cut to one line

pub struct Job<'a> {
    pub job: &'a str,
    pub try_: u64,
    pub tokens: u64,
    pub seconds: u64,
    pub files: &'a [String],
}

/// A TOML basic string: backslash and quote escaped, control characters dropped.
pub fn toml_string(s: &str, max: usize) -> String {
    let mut out = String::from("\"");
    for c in s.chars().take(max) {
        if c == '\\' || c == '"' {
            out.push('\\');
        }
        if c as u32 >= 0x20 {
            out.push(c);
        }
    }
    out.push('"');
    out
}

/// The code and the message of a line: the word after the first two spaces, and
/// what follows the time.
fn split(line: &str) -> Option<(&str, &str)> {
    let rest = &line[line.find("  ")? + 2..];
    let end = rest.find("  ")?;
    let code = &rest[..end];
    let after = &rest[end + 2..];
    let msg = after.find("  ").map(|i| &after[i + 2..]).unwrap_or("");
    Some((code, msg))
}

pub fn write(output: &str, job: &Job) -> String {
    let (mut passed, mut failed) = (0u64, 0u64);
    let mut names: Vec<&str> = Vec::new();
    let mut first = String::new();
    for raw in output.lines() {
        let s = raw.trim_end_matches('\r').trim_start_matches(' '); // the bench shows them indented
        if s.starts_with("PASS  ") {
            passed += 1;
        } else if s.starts_with("FAIL  ") {
            failed += 1;
            let Some((code, msg)) = split(s) else { continue };
            if failed == 1 {
                first = format!("{code}: {msg}");
            }
            if names.len() < MAX_NAMES {
                names.push(code);
            }
        }
    }
    let list = |v: &[&str], max: usize| v.iter().map(|x| toml_string(x, max)).collect::<Vec<_>>().join(", ");
    let mut n = String::new();
    n += &format!("job           = {}\n", toml_string(job.job, 64));
    n += &format!("status        = \"{}\"\n", if failed == 0 && passed > 0 { "pass" } else { "fail" });
    n += &format!("tests_passed  = {passed}\ntests_failed  = {failed}\nfailing       = [{}]\n", list(&names, 127));
    if failed > names.len() as u64 {
        n += &format!("failing_more  = {}\n", failed - names.len() as u64);
    }
    n += &format!("first_failure = {}\n", toml_string(&first, LINE));
    let files: Vec<&str> = job.files.iter().take(MAX_FILES).map(String::as_str).collect();
    n += &format!(
        "try           = {}\ntokens        = {}\nseconds       = {}\nfiles_changed = [{}]\n",
        job.try_, job.tokens, job.seconds, list(&files, 256)
    );
    if job.files.len() > MAX_FILES {
        n += &format!("files_more    = {}\n", job.files.len() - MAX_FILES);
    }
    n
}
