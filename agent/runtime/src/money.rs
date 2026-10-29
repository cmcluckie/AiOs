// SPDX-License-Identifier: GPL-2.0-only
//! Money: what a call costs and whether there is room for the next one.  Prices
//! are per million tokens, in and out, read from a price list of [[model]] rows
//! (name, per_million_input, per_million_output).  Whether the month has room is
//! asked of a command the caller names, so the runtime keeps no cap of its own and
//! there is one wall, wherever the caller keeps it.  A model with no price is never
//! costed as free: it has no price, and the caller does not call it.

use std::path::Path;
use std::process::Command;

pub fn price(list: &Path, model: &str) -> Option<(f64, f64)> {
    let text = std::fs::read_to_string(list).ok()?;
    let (mut hit, mut found) = (false, false);
    let (mut pin, mut pout) = (None, None);
    for line in text.lines() {
        let line = line.trim();
        if line.starts_with("[[model]]") {
            if found {
                break;
            }
            hit = false;
            continue;
        }
        let Some((k, v)) = line.split_once('=') else { continue };
        let (k, v) = (k.trim(), v.trim());
        match k {
            "name" => {
                hit = v.trim_matches('"') == model;
                found |= hit;
            }
            "per_million_input" if hit => pin = v.parse().ok(),
            "per_million_output" if hit => pout = v.parse().ok(),
            _ => {}
        }
    }
    Some((pin?, pout?))
}

/// Dollars for these tokens; None if the model has no price.
pub fn cost(list: &Path, model: &str, tokens_in: u64, tokens_out: u64) -> Option<f64> {
    let (i, o) = price(list, model)?;
    Some(tokens_in as f64 * i / 1e6 + tokens_out as f64 * o / 1e6)
}

/// Asks `command... CEILING` whether ceiling dollars more fit: exit 0 is yes.
/// Its first line of output says why, either way.
pub fn room(command: &[String], ceiling: f64) -> Result<String, String> {
    let Some((first, rest)) = command.split_first() else {
        return Err("no budget command".into());
    };
    let out = Command::new(first)
        .args(rest)
        .arg(format!("{ceiling:.6}"))
        .output()
        .map_err(|e| format!("the budget could not be asked: {e}"))?;
    let said = String::from_utf8_lossy(&out.stderr).to_string() + &String::from_utf8_lossy(&out.stdout);
    let line = said.lines().next().unwrap_or("").trim().to_string();
    if out.status.success() { Ok(line) } else { Err(if line.is_empty() { "the budget said no".into() } else { line }) }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn prices_come_from_the_named_row_only() {
        let dir = std::env::temp_dir().join(format!("money-{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        let list = dir.join("prices.toml");
        std::fs::write(&list, "[[model]]\nname = \"a\"\nper_million_input = 1.00\nper_million_output = 2.00\n\n[[model]]\nname = \"b\"\nper_million_input  = 3.00\nper_million_output = 15.00\n\n[[model]]\nname = \"c\"\nper_million_input = 1\n").unwrap();
        assert_eq!(price(&list, "b"), Some((3.0, 15.0)));
        assert!((cost(&list, "b", 1000, 100).unwrap() - 0.0045).abs() < 1e-12);
        assert_eq!(price(&list, "c"), None, "a row with no output price has no price");
        assert_eq!(price(&list, "d"), None, "no row, no price, never free");
    }

    #[test]
    fn room_is_the_commands_answer() {
        assert!(room(&["true".into()], 0.5).is_ok());
        assert!(room(&["false".into()], 0.5).is_err());
        assert!(room(&[], 0.5).is_err());
    }
}
