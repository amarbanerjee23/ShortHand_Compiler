#!/usr/bin/env bash
set -euo pipefail

REPORT="${1:-}"
[[ -n "${REPORT}" && -f "${REPORT}" ]] || { echo "usage: $0 <trivy-json-report>" >&2; exit 2; }
command -v jq >/dev/null 2>&1 || { echo "error: jq is required for Trivy report enforcement" >&2; exit 1; }
jq -e 'type == "object" and (.Results | type == "array")' "${REPORT}" >/dev/null || { echo "error: malformed Trivy JSON report" >&2; exit 1; }

vulns="$(jq '[.Results[]? | .Vulnerabilities[]? | select((.Severity // "") == "HIGH" or (.Severity // "") == "CRITICAL")] | length' "${REPORT}")"
secrets="$(jq '[.Results[]? | .Secrets[]?] | length' "${REPORT}")"
misconfigs="$(jq '[.Results[]? | .Misconfigurations[]? | select((.Severity // "") == "HIGH" or (.Severity // "") == "CRITICAL")] | length' "${REPORT}")"
licenses="$(jq '[.Results[]? | .Licenses[]? | select((.Severity // "") == "HIGH" or (.Severity // "") == "CRITICAL")] | length' "${REPORT}")"

if (( vulns > 0 || secrets > 0 || misconfigs > 0 || licenses > 0 )); then
  echo "error: Trivy policy rejected report vulnerabilities=${vulns} secrets=${secrets} misconfigurations=${misconfigs} licenses=${licenses}" >&2
  jq -r '.Results[]? as $r |
    ($r.Vulnerabilities[]? | select((.Severity // "") == "HIGH" or (.Severity // "") == "CRITICAL") | "vulnerability\t\(.VulnerabilityID)\t\(.PkgName)\t\(.Severity)"),
    ($r.Secrets[]? | "secret\t\(.RuleID // .Category // "unknown")\t\($r.Target // "")\t\(.Severity // "UNKNOWN")"),
    ($r.Misconfigurations[]? | select((.Severity // "") == "HIGH" or (.Severity // "") == "CRITICAL") | "misconfiguration\t\(.ID // .AVDID // "unknown")\t\($r.Target // "")\t\(.Severity)"),
    ($r.Licenses[]? | select((.Severity // "") == "HIGH" or (.Severity // "") == "CRITICAL") | "license\t\(.Name // .License // "unknown")\t\($r.Target // "")\t\(.Severity)")' "${REPORT}" >&2 || true
  exit 1
fi
printf 'PASS Trivy vulnerability and repository security policy gate vulnerabilities=%s secrets=%s misconfigurations=%s licenses=%s\n' "${vulns}" "${secrets}" "${misconfigs}" "${licenses}"
