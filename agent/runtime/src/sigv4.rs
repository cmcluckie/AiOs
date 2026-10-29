// SPDX-License-Identifier: GPL-2.0-only
//! AWS Signature Version 4, for Bedrock.  jig/model has curl sign its calls
//! (--aws-sigv4 "aws:amz:REGION:bedrock"); this is the same signature, written
//! here so the agent needs no AWS library.  It signs content-type, host and
//! x-amz-date (and the session token when there is one), as curl does, and is
//! checked against AWS's own published examples (the tests below).

use crate::transport::Signing;
use hmac::{Hmac, KeyInit, Mac};
use sha2::{Digest, Sha256};

type HmacSha256 = Hmac<Sha256>;

fn hmac(key: &[u8], data: &str) -> Vec<u8> {
    let mut m = HmacSha256::new_from_slice(key).expect("any key length");
    m.update(data.as_bytes());
    m.finalize().into_bytes().to_vec()
}

fn sha256_hex(data: &[u8]) -> String {
    hex::encode(Sha256::digest(data))
}

/// The time, as AWS writes it: 20150830T123600Z.
pub fn now() -> String {
    let secs = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0) as i64;
    let (y, mo, d) = civil(secs.div_euclid(86400));
    let t = secs.rem_euclid(86400);
    format!("{y:04}{mo:02}{d:02}T{:02}{:02}{:02}Z", t / 3600, t % 3600 / 60, t % 60)
}

/// Days since 1970-01-01 to year, month, day (Howard Hinnant's civil_from_days).
fn civil(z: i64) -> (i64, u32, u32) {
    let z = z + 719468;
    let era = z.div_euclid(146097);
    let doe = z - era * 146097;
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let y = yoe + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = (doy - (153 * mp + 2) / 5 + 1) as u32;
    let m = if mp < 10 { mp + 3 } else { mp - 9 } as u32;
    (if m <= 2 { y + 1 } else { y }, m, d)
}

/// Each byte that is not unreserved, %XX; '/' kept when it separates the path.
fn encode(s: &str, keep_slash: bool) -> String {
    let mut out = String::new();
    for b in s.bytes() {
        if b.is_ascii_alphanumeric() || b"-_.~".contains(&b) || (keep_slash && b == b'/') {
            out.push(b as char);
        } else {
            out.push_str(&format!("%{b:02X}"));
        }
    }
    out
}

/// host, path and query of an https URL
fn split(url: &str) -> (String, String, String) {
    let rest = url.split_once("://").map(|x| x.1).unwrap_or(url);
    let (host, path) = match rest.find('/') {
        Some(i) => (&rest[..i], &rest[i..]),
        None => (rest, "/"),
    };
    let (path, query) = path.split_once('?').unwrap_or((path, ""));
    (host.to_string(), path.to_string(), query.to_string())
}

fn canonical_query(q: &str) -> String {
    let mut pairs: Vec<(String, String)> = q
        .split('&')
        .filter(|p| !p.is_empty())
        .map(|p| {
            let (k, v) = p.split_once('=').unwrap_or((p, ""));
            (encode(k, false), encode(v, false))
        })
        .collect();
    pairs.sort();
    pairs.iter().map(|(k, v)| format!("{k}={v}")).collect::<Vec<_>>().join("&")
}

/// The signature and the signed headers' names for one request.  headers are the
/// ones to sign besides host and x-amz-date, as (lower-case name, value).
pub fn signature(
    s: &Signing,
    method: &str,
    url: &str,
    headers: &[(&str, &str)],
    body: &str,
    amz_date: &str,
) -> (String, String) {
    let (host, path, query) = split(url);
    let mut all: Vec<(String, String)> = headers.iter().map(|(k, v)| (k.to_lowercase(), v.trim().to_string())).collect();
    all.push(("host".into(), host));
    all.push(("x-amz-date".into(), amz_date.into()));
    if let Some(t) = s.session_token {
        all.push(("x-amz-security-token".into(), t.into()));
    }
    all.sort();
    let canonical_headers: String = all.iter().map(|(k, v)| format!("{k}:{v}\n")).collect();
    let signed: String = all.iter().map(|(k, _)| k.as_str()).collect::<Vec<_>>().join(";");
    let request = format!(
        "{method}\n{}\n{}\n{canonical_headers}\n{signed}\n{}",
        encode(&path, true),
        canonical_query(&query),
        sha256_hex(body.as_bytes())
    );
    let day = &amz_date[..8];
    let scope = format!("{day}/{}/{}/aws4_request", s.region, s.service);
    let to_sign = format!("AWS4-HMAC-SHA256\n{amz_date}\n{scope}\n{}", sha256_hex(request.as_bytes()));
    let k = hmac(format!("AWS4{}", s.secret_key).as_bytes(), day);
    let k = hmac(&k, s.region);
    let k = hmac(&k, s.service);
    let k = hmac(&k, "aws4_request");
    (hex::encode(hmac(&k, &to_sign)), signed)
}

/// The headers that sign a JSON POST: x-amz-date, the token if any, and Authorization.
pub fn headers(s: &Signing, method: &str, url: &str, body: &str, amz_date: &str) -> Vec<(String, String)> {
    let (sig, signed) = signature(s, method, url, &[("content-type", "application/json")], body, amz_date);
    let mut out = vec![("x-amz-date".to_string(), amz_date.to_string())];
    if let Some(t) = s.session_token {
        out.push(("x-amz-security-token".into(), t.into()));
    }
    out.push((
        "authorization".into(),
        format!(
            "AWS4-HMAC-SHA256 Credential={}/{}/{}/{}/aws4_request, SignedHeaders={signed}, Signature={sig}",
            s.access_key,
            &amz_date[..8],
            s.region,
            s.service
        ),
    ));
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    // AWS's published SigV4 test suite: AKIDEXAMPLE, us-east-1, service "service",
    // 2015-08-30 12:36:00 UTC, host example.amazonaws.com.
    fn example() -> Signing<'static> {
        Signing {
            region: "us-east-1",
            service: "service",
            access_key: "AKIDEXAMPLE",
            secret_key: "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY",
            session_token: None,
        }
    }

    #[test]
    fn get_vanilla() {
        let (sig, signed) = signature(&example(), "GET", "https://example.amazonaws.com/", &[], "", "20150830T123600Z");
        assert_eq!(signed, "host;x-amz-date");
        assert_eq!(sig, "5fa00fa31553b73ebf1942676e86291e8372ff2a2260956d9b8aae1d763fbf31");
    }

    #[test]
    fn post_vanilla() {
        let (sig, _) = signature(&example(), "POST", "https://example.amazonaws.com/", &[], "", "20150830T123600Z");
        assert_eq!(sig, "5da7c1a2acd57cee7505fc6676e4e544621c30862966e37dddb68e92efbe5d6b");
    }

    #[test]
    fn days_to_date() {
        assert_eq!(civil(0), (1970, 1, 1));
        assert_eq!(civil(16677), (2015, 8, 30));
        assert_eq!(civil(20755), (2026, 10, 29));
    }
}
