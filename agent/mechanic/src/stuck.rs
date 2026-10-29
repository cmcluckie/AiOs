// SPDX-License-Identifier: GPL-2.0-only
//! The stuck rule (findings 0027): the program, not the director, decides a part is
//! stuck, from the result notes alone.  Stuck when the last N runs in a row were
//! jobs on the same test and the same tests failed in each (stuck_same_failures;
//! every job starts from the plan's commit, so jobs on different tests can fail the
//! same way without being stuck), or when the failing count in the last M runs has
//! not gone below the lowest it was before them (stuck_no_drop).  A pass starts the
//! count again.  A stuck part stops and goes to strategic; the director cannot talk
//! its way past it.

#[derive(Clone, Debug, Default)]
pub struct Run {
    pub test: String,
    pub failing: Vec<String>, // the note's failing tests, sorted
    pub failed: u64,          // the note's count, named or not
    pub passed: bool,         // the job's test passed
}

/// A note's failing list and count.
pub fn from_note(note: &str, test: &str, passed: bool) -> Option<Run> {
    let t: toml::Table = toml::from_str(note).ok()?;
    let failed = t.get("tests_failed")?.as_integer()? as u64;
    let mut failing: Vec<String> =
        t.get("failing")?.as_array()?.iter().filter_map(|v| v.as_str().map(String::from)).collect();
    failing.sort();
    Some(Run { test: test.into(), failing, failed, passed })
}

fn same(a: &Run, b: &Run) -> bool {
    a.test == b.test && a.failing == b.failing && a.failed == b.failed
}

pub fn check(runs: &[Run], same_n: u64, nodrop: u64) -> Option<String> {
    let start = runs.iter().rposition(|r| r.passed).map(|i| i + 1).unwrap_or(0);
    let runs = &runs[start..];
    let (n, same_n, nodrop) = (runs.len(), same_n as usize, nodrop as usize);
    if same_n > 0 && n >= same_n {
        let window = &runs[n - same_n..];
        if window.iter().all(|r| same(r, &window[0])) {
            let last = &runs[n - 1];
            return Some(format!(
                "the same {} test(s) failed in the last {same_n} runs in a row on {}{}{}",
                last.failed,
                last.test,
                if last.failing.is_empty() { "" } else { ": " },
                last.failing.join(" ")
            ));
        }
    }
    if nodrop > 0 && n > nodrop {
        let low_before = runs[..n - nodrop].iter().map(|r| r.failed).min().unwrap_or(0);
        let low_last = runs[n - nodrop..].iter().map(|r| r.failed).min().unwrap_or(0);
        if low_last >= low_before {
            return Some(format!("the failing count has not gone below {low_before} in the last {nodrop} runs"));
        }
    }
    None
}
