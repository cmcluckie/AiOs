// SPDX-License-Identifier: GPL-2.0-only
//! The prompt library: one file per role and version, NAME@N.txt in one folder,
//! named by the plan that uses it.  Each version is its own file and never written
//! over; its fingerprint is git's own (the blob's hash, first eight), so each
//! version is its own line on the scorecard, as jig/fix records its prompt.

use sha1::{Digest, Sha1};
use std::path::Path;

/// A library name, NAME@N: letters, digits, - and _, an @ and a version.
pub fn valid(name: &str) -> bool {
    match name.split_once('@') {
        Some((n, v)) => {
            !n.is_empty()
                && n.chars().all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_')
                && !v.is_empty()
                && v.chars().all(|c| c.is_ascii_digit())
        }
        None => false,
    }
}

/// The prompt's text and fingerprint, or why there is none.
pub fn load(dir: &Path, name: &str) -> Result<(String, String), String> {
    if !valid(name) {
        return Err(format!("{name} is not a prompt's name and version, NAME@N"));
    }
    let path = dir.join(format!("{name}.txt"));
    let text = std::fs::read_to_string(&path).map_err(|_| format!("no prompt {name} at {}", path.display()))?;
    let print = fingerprint(&text);
    Ok((text, print))
}

/// git hash-object's id, first eight characters.
pub fn fingerprint(text: &str) -> String {
    let mut h = Sha1::new();
    h.update(format!("blob {}\0", text.len()).as_bytes());
    h.update(text.as_bytes());
    hex::encode(h.finalize())[..8].to_string()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn fingerprint_is_gits() {
        // printf 'hello\n' | git hash-object --stdin
        assert_eq!(fingerprint("hello\n"), "ce013625");
    }

    #[test]
    fn names_carry_a_version() {
        assert!(valid("builder@1"));
        assert!(!valid("builder"));
        assert!(!valid("../x@1"));
        assert!(!valid("builder@"));
    }
}
