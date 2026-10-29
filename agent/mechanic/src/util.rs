// SPDX-License-Identifier: GPL-2.0-only
//! Small things more than one piece needs.

/// Only letters, digits and the characters in also: safe on a command line.
pub fn plain(s: &str, also: &str) -> bool {
    s.chars().all(|c| c.is_ascii_alphanumeric() || also.contains(c))
}
