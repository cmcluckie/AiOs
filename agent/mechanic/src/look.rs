// SPDX-License-Identifier: GPL-2.0-only
//! The builder's look tool: list, read and find, answered from the builder's own
//! copy of the repo and nothing outside it.  The same three commands, limits and
//! answers jig/fix's look gives the mechanic today (decisions 0060 and 0092), so
//! the two benches hand a model the same eyes.

use std::fs;
use std::path::Path;

pub const MAX_LINES: u64 = 400;
pub const MAX_HITS: usize = 100;
pub const MAX_LIST: usize = 200;

/// A path the model gave that could reach outside the copy.
fn outside(a: &str) -> bool {
    a.starts_with('/')
        || a == ".."
        || a.starts_with("../")
        || a.ends_with("/..")
        || a.contains("/../")
        || a == ".git"
        || a.starts_with(".git/")
}

/// The answer, and whether it is one (false: an error the model is told).
pub fn look(base: &Path, command: &str, args: &[String]) -> (String, bool) {
    if let Some(a) = args.iter().find(|a| outside(a)) {
        return (format!("refused: {a} is outside the repo"), false);
    }
    match command {
        "list" => list(base, args.first().map(String::as_str).unwrap_or(".")),
        "read" if args.is_empty() => ("no file".into(), false),
        "read" => read(base, args),
        "find" if args.is_empty() => ("find needs a word".into(), false),
        "find" => find(base, args),
        _ => ("the look tool has list, read and find".into(), false),
    }
}

fn list(base: &Path, dir: &str) -> (String, bool) {
    let Ok(rd) = fs::read_dir(base.join(dir)) else {
        return (format!("no folder {dir}"), false);
    };
    let mut names: Vec<String> = rd
        .filter_map(Result::ok)
        .map(|e| {
            let n = e.file_name().to_string_lossy().to_string();
            // symlink_metadata: a link is listed as itself, never followed
            if e.path().symlink_metadata().map(|m| m.is_dir()).unwrap_or(false) { n + "/" } else { n }
        })
        .collect();
    names.sort();
    (names.iter().take(MAX_LIST).map(|n| format!("{n}\n")).collect(), true)
}

fn number(s: &str) -> Option<u64> {
    if s.is_empty() || !s.chars().all(|c| c.is_ascii_digit()) { None } else { s.parse().ok() }
}

/// FILE, FILE FIRST, FILE FIRST LAST or FILE FIRST-LAST
fn read(base: &Path, args: &[String]) -> (String, bool) {
    let path = base.join(&args[0]);
    let is_file = path.symlink_metadata().map(|m| m.is_file()).unwrap_or(false);
    let Some(text) = is_file.then(|| fs::read(&path).ok()).flatten() else {
        return (format!("no file {}", args[0]), false);
    };
    let text = String::from_utf8_lossy(&text);
    let (first, last) = match args.get(1) {
        Some(r) if r.find('-').map(|i| i > 0).unwrap_or(false) => {
            let (a, b) = r.split_once('-').unwrap();
            (Some(a.to_string()), Some(b.to_string()))
        }
        Some(a) => (Some(a.clone()), args.get(2).cloned()),
        None => (None, None),
    };
    let a = match first.as_deref().map(number) {
        Some(None) => return ("lines are numbers: FILE FIRST LAST".into(), false),
        Some(Some(n)) => n,
        None => 1,
    };
    let mut b = match last.as_deref().map(number) {
        Some(None) => return ("lines are numbers: FILE FIRST LAST".into(), false),
        Some(Some(n)) => n,
        None => a + MAX_LINES - 1,
    };
    if b < a {
        return ("the last line comes before the first".into(), false);
    }
    if b - a >= MAX_LINES {
        b = a + MAX_LINES - 1;
    }
    let lines: Vec<&str> = text.lines().collect();
    let n = lines.len() as u64;
    if n == 0 || a > n {
        return (format!("{} has {n} lines", args[0]), false);
    }
    b = b.min(n);
    let mut out = format!("{}, lines {a} to {b} of {n}\n", args[0]);
    for i in a..=b {
        out += &format!("{i}\t{}\n", lines[(i - 1) as usize]);
    }
    (out, true)
}

/// every file under dir, links not followed, in name order
fn walk(dir: &Path, out: &mut Vec<std::path::PathBuf>) {
    let Ok(rd) = fs::read_dir(dir) else { return };
    let mut entries: Vec<_> = rd.filter_map(Result::ok).collect();
    entries.sort_by_key(|e| e.file_name());
    for e in entries {
        let Ok(m) = e.path().symlink_metadata() else { continue };
        if m.is_dir() && e.file_name() != ".git" {
            walk(&e.path(), out);
        } else if m.is_file() {
            out.push(e.path());
        }
    }
}

/// WORD, or WORD FOLDER-OR-FILE; as grep -rnF would answer, in name order
fn find(base: &Path, args: &[String]) -> (String, bool) {
    let mut files = Vec::new();
    if let Some(where_) = args.get(1) {
        let p = base.join(where_);
        match p.symlink_metadata() {
            Ok(m) if m.is_file() => files.push(p),
            Ok(m) if m.is_dir() => walk(&p, &mut files),
            _ => return (format!("no folder or file {where_}"), false),
        }
    } else {
        for d in ["pool", "specs", "tests", "index", "findings", "jig"] {
            walk(&base.join(d), &mut files);
        }
    }
    let word = &args[0];
    let mut hits = Vec::new();
    for f in &files {
        let Ok(bytes) = fs::read(f) else { continue };
        let text = String::from_utf8_lossy(&bytes);
        let rel = f.strip_prefix(base).unwrap_or(f).to_string_lossy().to_string();
        for (i, line) in text.lines().enumerate() {
            if line.contains(word.as_str()) {
                hits.push(format!("{rel}:{}:{line}\n", i + 1));
            }
        }
    }
    (hits.iter().take(MAX_HITS).cloned().collect(), true)
}
