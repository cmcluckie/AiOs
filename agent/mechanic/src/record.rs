// SPDX-License-Identifier: GPL-2.0-only
//! Run records and money, AiOs's way: every run goes into runs/ through jig/runs
//! record, as every other run in the repo does, with the bench, its peak context
//! and how it ended (SPEC.exam.10), so the month's spend and the scorecard count
//! the agent's runs beside the old bench's.  Prices are jig/prices.toml's and the
//! month's cap jig/budget.toml's, asked through jig/runs budget, one wall for both
//! benches.  The keys for Bedrock are read from the file jig/model reads.

use std::path::{Path, PathBuf};
use std::process::Command;

pub struct Runs {
    pub command: Vec<String>, // jig/runs, or AGENT_RUNS
    pub prices: PathBuf,      // PRICES, or the repo's jig/prices.toml
}

impl Runs {
    pub fn new(repo: &Path) -> Runs {
        let command = match std::env::var("AGENT_RUNS") {
            Ok(v) if !v.trim().is_empty() => v.split_whitespace().map(String::from).collect(),
            _ => vec!["sh".into(), repo.join("jig/runs").to_string_lossy().to_string()],
        };
        let prices = std::env::var("PRICES").ok().filter(|v| !v.is_empty()).map(PathBuf::from).unwrap_or_else(|| repo.join("jig/prices.toml"));
        Runs { command, prices }
    }

    pub fn cost(&self, model: &str, tin: u64, tout: u64) -> Option<f64> {
        runtime::money::cost(&self.prices, model, tin, tout)
    }

    /// Room this month for ceiling more dollars; why not, if not.
    pub fn room(&self, ceiling: f64) -> Result<(), String> {
        let mut c = self.command.clone();
        c.push("budget".into());
        runtime::money::room(&c, ceiling).map(|_| ())
    }

    /// jig/runs record, with the agent's fields; the run's id, or why not.
    #[allow(clippy::too_many_arguments)]
    pub fn record(
        &self,
        item: &str,
        model: &str,
        try_: u64,
        tin: u64,
        tout: u64,
        result: &str,
        lines: u64,
        input: &Path,
        output: &Path,
        at: &str,
        prompt: &str,
        sitting: Option<&str>,
        peak: u64,
        ended: &str,
        trial: &str,
    ) -> Result<String, String> {
        let day = runtime::sigv4::now();
        let day = format!("{}-{}-{}", &day[..4], &day[4..6], &day[6..8]);
        let mut c = Command::new(&self.command[0]);
        c.args(&self.command[1..])
            .args(["record", item, model, &day, &try_.to_string(), &tin.to_string(), &tout.to_string(), result, &lines.to_string()])
            .arg(input)
            .arg(output)
            .env("RUNS_BENCH", "agent")
            .env("RUNS_PEAK", peak.to_string())
            .env("RUNS_ENDED", ended)
            .env("RUNS_AT", at)
            .env("RUNS_PROMPT", prompt);
        if let Some(s) = sitting {
            c.env("RUNS_SITTING", s);
        }
        if !trial.is_empty() {
            c.env("RUNS_TRIAL", trial);
        }
        let out = c.output().map_err(|e| format!("jig/runs: {e}"))?;
        if out.status.success() {
            Ok(String::from_utf8_lossy(&out.stdout).trim().to_string())
        } else {
            Err(String::from_utf8_lossy(&out.stderr).trim().to_string())
        }
    }
}

/// The Bedrock key, from AIOS_AWS or ~/.aios/aws, refused if anyone but its owner
/// can read it (as jig/model refuses it).
pub fn keys() -> Result<runtime::model::Keys, String> {
    use std::os::unix::fs::PermissionsExt;
    let path = std::env::var("AIOS_AWS")
        .ok()
        .filter(|v| !v.is_empty())
        .unwrap_or_else(|| format!("{}/.aios/aws", std::env::var("HOME").unwrap_or_default()));
    let meta = std::fs::metadata(&path).map_err(|_| format!("no key file at {path}"))?;
    if meta.permissions().mode() & 0o077 != 0 {
        return Err(format!("{path} can be read by others; chmod 600 it"));
    }
    let text = std::fs::read_to_string(&path).map_err(|_| format!("no key file at {path}"))?;
    let (mut id, mut secret, mut token, mut region) = (None, None, None, None);
    for line in text.lines() {
        let line = line.trim().trim_start_matches("export ");
        let Some((k, v)) = line.split_once('=') else { continue };
        let v = v.trim().trim_matches('"').trim_matches('\'').to_string();
        match k.trim() {
            "AWS_ACCESS_KEY_ID" => id = Some(v),
            "AWS_SECRET_ACCESS_KEY" => secret = Some(v),
            "AWS_SESSION_TOKEN" => token = Some(v),
            "AWS_REGION" => region = Some(v),
            _ => {}
        }
    }
    match (id, secret, region) {
        (Some(access_key), Some(secret_key), Some(region)) => {
            Ok(runtime::model::Keys { access_key, secret_key, session_token: token, region })
        }
        _ => Err(format!("{path} needs AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY and AWS_REGION")),
    }
}
