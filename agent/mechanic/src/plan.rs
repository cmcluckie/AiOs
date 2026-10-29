// SPDX-License-Identifier: GPL-2.0-only
//! The part plan: what strategic (Chris and Claude) hands a director (findings
//! 0027, SPEC.agent.2).  One TOML file; every limit the director runs under is in
//! it, and nothing on the card can change it.  A plan missing a field, with a key
//! no plan has, or a value of the wrong kind or out of range is refused, naming
//! the field, and nothing runs.

use toml::{Table, Value};

pub const MAX_TESTS: usize = 16;

#[derive(Clone, Debug)]
pub struct Plan {
    pub part: String,
    pub petition: String,
    pub tests: Vec<String>,
    pub at: String,
    pub gear_tactical: u8,
    pub gear_builders: u8,
    pub budget_usd: f64,
    pub max_tries: u64,
    pub max_minutes: u64,
    pub stuck_same_failures: u64,
    pub stuck_no_drop: u64,
    pub prompt_director: String,
    pub prompt_builder: String,
    pub trial: String, // a prompt trial this run belongs to, "" for none
}

/// toml's own words for what it could not read, said plainly with the line
pub fn toml_error(path: &str, text: &str, e: &toml::de::Error) -> String {
    let line = e.span().map(|s| text[..s.start.min(text.len())].lines().count().max(1)).unwrap_or(0);
    let said = e.message().to_string();
    let plain = if said.contains("duplicate key") {
        "a key given twice"
    } else if said.contains("string") && (said.contains("unterminated") || said.contains("invalid basic string") || said.contains("newline")) {
        "a string not closed"
    } else {
        "not TOML the repo writes"
    };
    format!("{path} line {line}: {plain} ({})", said.trim())
}

struct Reader {
    t: Table,
    used: Vec<String>,
}

impl Reader {
    fn get(&mut self, key: &str) -> Option<&Value> {
        self.used.push(key.to_string());
        let mut v: Option<&Value> = None;
        for (i, part) in key.split('.').enumerate() {
            v = if i == 0 { self.t.get(part) } else { v?.get(part) };
        }
        v
    }
    fn text(&mut self, key: &str, need: bool) -> Result<String, String> {
        match self.get(key) {
            Some(Value::String(s)) => Ok(s.clone()),
            Some(_) => Err(format!("{key} is a string")),
            None if need => Err(format!("{key} is missing")),
            None => Ok(String::new()),
        }
    }
    fn whole(&mut self, key: &str, default: Option<u64>, lo: u64, hi: u64) -> Result<u64, String> {
        let v = match self.get(key) {
            Some(Value::Integer(n)) => *n,
            Some(_) => return Err(format!("{key} is a whole number")),
            None => match default {
                Some(d) => return Ok(d),
                None => return Err(format!("{key} is missing")),
            },
        };
        if v < lo as i64 || v > hi as i64 {
            return Err(format!("{key} is {v}, outside {lo} to {hi}"));
        }
        Ok(v as u64)
    }
    /// the first key nobody asked for
    fn unused(&self) -> Option<String> {
        for (k, v) in &self.t {
            match v {
                Value::Table(sub) => {
                    for k2 in sub.keys() {
                        let full = format!("{k}.{k2}");
                        if !self.used.contains(&full) {
                            return Some(full);
                        }
                    }
                }
                _ if !self.used.contains(k) => return Some(k.clone()),
                _ => {}
            }
        }
        None
    }
}

fn version(s: &str) -> bool {
    runtime::prompts::valid(s)
}

pub fn read(path: &str) -> Result<Plan, String> {
    let text = std::fs::read_to_string(path).map_err(|_| format!("{path} could not be read"))?;
    let t: Table = toml::from_str(&text).map_err(|e| toml_error(path, &text, &e))?;
    let mut r = Reader { t, used: vec![] };
    let part = r.text("part", true)?;
    let petition = r.text("petition", false)?;
    let at = r.text("at", true)?;
    if at.len() < 7 || !at.chars().all(|c| c.is_ascii_hexdigit() && !c.is_ascii_uppercase()) {
        return Err("at is a commit's hash, seven hex digits or more".into());
    }
    let tests = match r.get("tests") {
        None => return Err("tests is missing".into()),
        Some(Value::Array(a)) if !a.is_empty() && a.len() <= MAX_TESTS => a.clone(),
        Some(_) => return Err(format!("tests is a list of one to {MAX_TESTS} tests")),
    };
    let mut names = Vec::new();
    for t in tests {
        match t {
            Value::String(s) if s.contains('@') => names.push(s),
            _ => return Err("tests names each test with its version, like share.waits@1".into()),
        }
    }
    let gear_tactical = r.whole("gear.tactical", None, 1, 2)? as u8;
    let gear_builders = r.whole("gear.builders", None, 1, 2)? as u8;
    let budget_usd = match r.get("budget_usd") {
        None => return Err("budget_usd is missing".into()),
        Some(Value::Float(f)) if *f >= 0.0 => *f,
        Some(Value::Integer(n)) if *n >= 0 => *n as f64,
        Some(_) => return Err("budget_usd is a number of dollars, 0 or more".into()),
    };
    let max_tries = r.whole("max_tries", Some(10), 1, 1000)?;
    let max_minutes = r.whole("max_minutes", Some(120), 1, 10080)?;
    let stuck_same_failures = r.whole("stuck_same_failures", Some(3), 2, 100)?;
    let stuck_no_drop = r.whole("stuck_no_drop", Some(5), 1, 100)?;
    let prompt_director = r.text("prompts.director", true)?;
    let prompt_builder = r.text("prompts.builder", true)?;
    // a prompt trial: two plans alike but for their prompts share one name, and the
    // scorecard shows their rows together, marked (findings 0029)
    let trial = r.text("trial", false)?;
    if !trial.chars().all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_') {
        return Err("trial is a name: letters, digits, - and _".into());
    }
    if !version(&prompt_director) || !version(&prompt_builder) {
        return Err("prompts name a library version, like builder@1".into());
    }
    if let Some(k) = r.unused() {
        return Err(format!("{k} is not a field of a part plan"));
    }
    Ok(Plan {
        part,
        petition,
        tests: names,
        at,
        gear_tactical,
        gear_builders,
        budget_usd,
        max_tries,
        max_minutes,
        stuck_same_failures,
        stuck_no_drop,
        prompt_director,
        prompt_builder,
        trial,
    })
}

/// The plan written back, every default filled in.
pub fn write(p: &Plan) -> String {
    let q = |k: &str, v: &str| format!("{k:<19} = \"{v}\"\n");
    let tests = p.tests.iter().map(|t| format!("\"{t}\"")).collect::<Vec<_>>().join(", ");
    let mut s = q("part", &p.part) + &q("petition", &p.petition);
    s += &format!("{:<19} = [{tests}]\n", "tests");
    s += &q("at", &p.at);
    s += &format!("{:<19} = {:.2}\n", "budget_usd", p.budget_usd);
    s += &format!(
        "{:<19} = {}\n{:<19} = {}\n{:<19} = {}\n{:<19} = {}\n",
        "max_tries", p.max_tries, "max_minutes", p.max_minutes, "stuck_same_failures", p.stuck_same_failures, "stuck_no_drop", p.stuck_no_drop
    );
    if !p.trial.is_empty() {
        s += &q("trial", &p.trial);
    }
    s += &format!("\n[gear]\n{:<19} = {}\n{:<19} = {}\n", "tactical", p.gear_tactical, "builders", p.gear_builders);
    s += "\n[prompts]\n";
    s += &q("director", &p.prompt_director);
    s += &q("builder", &p.prompt_builder);

    s
}
