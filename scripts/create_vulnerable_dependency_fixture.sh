#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-/tmp/shorthand-vulnerable-dependency-fixture}"
case "${OUT}" in /tmp/*) ;; *) echo "error: vulnerable scanner fixture must be generated under /tmp" >&2; exit 1 ;; esac
rm -rf "${OUT}"
mkdir -p "${OUT}"
cat >"${OUT}/package.json" <<'JSON'
{
  "name": "shorthand-security-negative-fixture",
  "version": "1.0.0",
  "private": true,
  "dependencies": {
    "lodash": "4.17.15"
  }
}
JSON
cat >"${OUT}/package-lock.json" <<'JSON'
{
  "name": "shorthand-security-negative-fixture",
  "version": "1.0.0",
  "lockfileVersion": 3,
  "requires": true,
  "packages": {
    "": {
      "name": "shorthand-security-negative-fixture",
      "version": "1.0.0",
      "dependencies": {
        "lodash": "4.17.15"
      }
    },
    "node_modules/lodash": {
      "version": "4.17.15",
      "license": "MIT"
    }
  }
}
JSON
printf 'Generated vulnerable dependency scanner fixture at %s\n' "${OUT}"
