// SPDX-License-Identifier: GPL-2.0-only
//! The one way out of the agent: an HTTP POST of a JSON body.  The real one is
//! reqwest; with the `fake` feature it is a folder of saved replies instead
//! (AGENT_FAKE: 1.json, 2.json and on, handed back in turn, each body sent kept
//! as sent-N.json and where it went as sent-N.url, the count in the folder so a
//! director and the builders it starts share one sequence).  Nothing else in the
//! agent reaches the network.

use std::time::Duration;

/// A request signed for AWS (Bedrock): region and service; the keys are given
/// with it, never read here.
pub struct Signing<'a> {
    pub region: &'a str,
    pub service: &'a str,
    pub access_key: &'a str,
    pub secret_key: &'a str,
    pub session_token: Option<&'a str>,
}

#[derive(Debug)]
pub enum Failure {
    /// nothing answered: no model there, or no network
    Unreachable(String),
    /// something answered, and it was not a reply
    Other(String),
}

/// POSTs body to url; the status and the body that came back.
pub fn post(url: &str, body: &str, sign: Option<&Signing>, timeout: Duration) -> Result<(u16, String), Failure> {
    imp::post(url, body, sign, timeout)
}

#[cfg(not(feature = "fake"))]
mod imp {
    use super::*;

    pub fn post(url: &str, body: &str, sign: Option<&Signing>, timeout: Duration) -> Result<(u16, String), Failure> {
        let client = reqwest::blocking::Client::builder()
            .timeout(timeout)
            .connect_timeout(Duration::from_secs(10))
            .build()
            .map_err(|e| Failure::Other(e.to_string()))?;
        let mut req = client
            .post(url)
            .header("content-type", "application/json")
            .header("accept", "application/json");
        if let Some(s) = sign {
            for (k, v) in crate::sigv4::headers(s, "POST", url, body, &crate::sigv4::now()) {
                req = req.header(k, v);
            }
        }
        let resp = req.body(body.to_string()).send().map_err(|e| {
            if e.is_connect() || e.is_timeout() && !e.is_body() {
                Failure::Unreachable(e.to_string())
            } else {
                Failure::Other(e.to_string())
            }
        })?;
        let status = resp.status().as_u16();
        let text = resp.text().map_err(|e| Failure::Other(e.to_string()))?;
        Ok((status, text))
    }
}

#[cfg(feature = "fake")]
mod imp {
    use super::*;
    use std::fs;

    pub fn post(url: &str, body: &str, sign: Option<&Signing>, _timeout: Duration) -> Result<(u16, String), Failure> {
        let dir = std::env::var("AGENT_FAKE").map_err(|_| Failure::Unreachable("no AGENT_FAKE".into()))?;
        let count = format!("{dir}/calls");
        let n: u32 = fs::read_to_string(&count).ok().and_then(|s| s.trim().parse().ok()).unwrap_or(0) + 1;
        let _ = fs::write(&count, format!("{n}\n"));
        let _ = fs::write(format!("{dir}/sent-{n}.json"), body);
        let how = sign.map(|s| format!("aws:amz:{}:{}", s.region, s.service)).unwrap_or_else(|| "-".into());
        let _ = fs::write(format!("{dir}/sent-{n}.url"), format!("{url} {how}\n"));
        match fs::read_to_string(format!("{dir}/{n}.json")) {
            Ok(text) => Ok((200, text)),
            Err(_) => Err(Failure::Unreachable(format!("no saved reply {n}"))),
        }
    }
}
