#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
SHORTHAND_BIN="${SHORTHAND_BIN:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand}"
CXX="${CXX:-g++}"
LSP_FLAGS_TEXT="${SHORTHAND_LSP_CXXFLAGS:--std=c++17 -O2 -Wall -Wextra -Wpedantic}"
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

for tool in "${CXX}" jq grep timeout wc tr; do
  command -v "${tool}" >/dev/null 2>&1 || {
    echo "error: required LSP/editor qualification tool unavailable: ${tool}" >&2
    exit 1
  }
done
[[ -x "${SHORTHAND_BIN}" ]] || {
  echo "error: ShortHand compiler unavailable for compiler-backed LSP diagnostics: ${SHORTHAND_BIN}" >&2
  exit 1
}

LSP="${TMP}/shorthand_lsp"
make -C "${SRC_DIR}/tooling" \
  lsp \
  LSP_BIN="${LSP}" \
  CXX="${CXX}" \
  CXXFLAGS="${LSP_FLAGS_TEXT}"
[[ -x "${LSP}" ]] || { echo "error: LSP build did not produce an executable" >&2; exit 1; }

# Editor artifacts are machine-parseable and advertise the real .short language scope.
for json in \
  "${ROOT_DIR}/editors/vscode/package.json" \
  "${ROOT_DIR}/editors/vscode/language-configuration.json" \
  "${ROOT_DIR}/editors/vscode/syntaxes/shorthand.tmLanguage.json"; do
  jq -e . "${json}" >/dev/null
done

grep -Fq '"scopeName": "source.shorthand"' "${ROOT_DIR}/editors/vscode/syntaxes/shorthand.tmLanguage.json"
grep -Fq 'greenai_contract' "${ROOT_DIR}/editors/vscode/syntaxes/shorthand.tmLanguage.json"
grep -Fq 'certification' "${ROOT_DIR}/editors/vscode/syntaxes/shorthand.tmLanguage.json"
grep -Fq 'certification_profile' "${ROOT_DIR}/editors/vscode/syntaxes/shorthand.tmLanguage.json"
grep -Fq 'measurement_plan' "${ROOT_DIR}/editors/vscode/syntaxes/shorthand.tmLanguage.json"
grep -Fq 'onnxruntime_cpu' "${ROOT_DIR}/editors/vscode/syntaxes/shorthand.tmLanguage.json"
grep -Fq 'comment.line.number-sign.shorthand' "${ROOT_DIR}/editors/vscode/syntaxes/shorthand.tmLanguage.json"
grep -Fq '"extensions": [".short"]' "${ROOT_DIR}/editors/vscode/package.json"

send_message() {
  local body="$1"
  local bytes
  bytes="$(LC_ALL=C printf '%s' "${body}" | wc -c | tr -d ' ')"
  printf 'Content-Length: %s\r\n\r\n%s' "${bytes}" "${body}"
}

APP="${ROOT_DIR}/tests/modules/resolver/valid_project/src/app.short"
APP_URI="file://${APP}"
APP_JSON="$(jq -Rs . <"${APP}")"
PARTIAL_JSON='"@"'
RECOVERED_JSON='"int value;\n"'

# Full protocol/golden session. A single bounded process proves compiler-backed
# diagnostics, completion, hover, document symbols, imported definition,
# cancellation, partial-document recovery and clean shutdown.
{
  send_message '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{"general":{"positionEncodings":["utf-16"]}}}}'
  send_message '{"jsonrpc":"2.0","method":"initialized","params":{}}'
  send_message "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"${APP_URI}\",\"languageId\":\"shorthand\",\"version\":1,\"text\":${APP_JSON}}}}"
  send_message "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"textDocument/completion\",\"params\":{\"textDocument\":{\"uri\":\"${APP_URI}\"},\"position\":{\"line\":4,\"character\":2}}}"
  send_message "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"textDocument/hover\",\"params\":{\"textDocument\":{\"uri\":\"${APP_URI}\"},\"position\":{\"line\":2,\"character\":25}}}"
  send_message "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"textDocument/definition\",\"params\":{\"textDocument\":{\"uri\":\"${APP_URI}\"},\"position\":{\"line\":2,\"character\":25}}}"
  send_message "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"textDocument/documentSymbol\",\"params\":{\"textDocument\":{\"uri\":\"${APP_URI}\"}}}"
  send_message '{"jsonrpc":"2.0","method":"$/cancelRequest","params":{"id":99}}'
  send_message "{\"jsonrpc\":\"2.0\",\"id\":99,\"method\":\"textDocument/completion\",\"params\":{\"textDocument\":{\"uri\":\"${APP_URI}\"},\"position\":{\"line\":4,\"character\":2}}}"
  send_message "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\",\"params\":{\"textDocument\":{\"uri\":\"${APP_URI}\",\"version\":2},\"contentChanges\":[{\"text\":${PARTIAL_JSON}}]}}"
  send_message "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\",\"params\":{\"textDocument\":{\"uri\":\"${APP_URI}\",\"version\":3},\"contentChanges\":[{\"text\":${RECOVERED_JSON}}]}}"
  send_message "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didClose\",\"params\":{\"textDocument\":{\"uri\":\"${APP_URI}\"}}}"
  send_message '{"jsonrpc":"2.0","id":6,"method":"shutdown","params":null}'
  send_message '{"jsonrpc":"2.0","method":"exit","params":null}'
} | SHORTHAND_COMPILER="${SHORTHAND_BIN}" timeout 8s "${LSP}" >"${TMP}/protocol.out" 2>"${TMP}/protocol.err"

PROTOCOL="${TMP}/protocol.out"
grep -Fq '"name":"shorthand-lsp"' "${PROTOCOL}"
grep -Fq '"positionEncoding":"utf-16"' "${PROTOCOL}"
grep -Fq '"method":"textDocument/publishDiagnostics"' "${PROTOCOL}"
grep -Fq '"diagnostics":[]' "${PROTOCOL}"
grep -Fq '"source":"shorthand-compiler"' "${PROTOCOL}"
grep -Fq '"label":"greenai_contract"' "${PROTOCOL}"
grep -Fq '"label":"certification"' "${PROTOCOL}"
grep -Fq '"label":"certification_profile"' "${PROTOCOL}"
grep -Fq '"label":"guardrails"' "${PROTOCOL}"
grep -Fq 'ShortHand source symbol' "${PROTOCOL}"
grep -Fq 'src/lib.short' "${PROTOCOL}"
grep -Fq '"name":"acme.demo.app"' "${PROTOCOL}"
grep -Fq '"code":-32800' "${PROTOCOL}"
grep -Fq '"version":3' "${PROTOCOL}"

# UTF-16 positions must not accidentally use UTF-8 byte offsets. The emoji is
# two UTF-16 code units but four UTF-8 bytes; `value` still begins at unit 16.
printf 'print "😀"; int value;\nprint value;\n' >"${TMP}/unicode.short"
UNICODE_URI="file://${TMP}/unicode.short"
UNICODE_JSON="$(jq -Rs . <"${TMP}/unicode.short")"
{
  send_message '{"jsonrpc":"2.0","id":10,"method":"initialize","params":{}}'
  send_message "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"${UNICODE_URI}\",\"languageId\":\"shorthand\",\"version\":1,\"text\":${UNICODE_JSON}}}}"
  send_message "{\"jsonrpc\":\"2.0\",\"id\":11,\"method\":\"textDocument/definition\",\"params\":{\"textDocument\":{\"uri\":\"${UNICODE_URI}\"},\"position\":{\"line\":1,\"character\":8}}}"
  send_message '{"jsonrpc":"2.0","id":12,"method":"shutdown","params":null}'
  send_message '{"jsonrpc":"2.0","method":"exit","params":null}'
} | SHORTHAND_COMPILER="${SHORTHAND_BIN}" timeout 8s "${LSP}" >"${TMP}/unicode.out" 2>"${TMP}/unicode.err"
grep -Fq '"start":{"character":16,"line":0}' "${TMP}/unicode.out"
grep -Fq '"end":{"character":21,"line":0}' "${TMP}/unicode.out"

# Malformed JSON produces a JSON-RPC parse error but does not poison the server.
{
  send_message '{"jsonrpc":'
  send_message '{"jsonrpc":"2.0","id":20,"method":"initialize","params":{}}'
  send_message '{"jsonrpc":"2.0","id":21,"method":"shutdown","params":null}'
  send_message '{"jsonrpc":"2.0","method":"exit","params":null}'
} | SHORTHAND_COMPILER="${SHORTHAND_BIN}" timeout 8s "${LSP}" >"${TMP}/parse_error.out" 2>"${TMP}/parse_error.err"
grep -Fq '"code":-32700' "${TMP}/parse_error.out"
grep -Fq '"id":20' "${TMP}/parse_error.out"

# Framing is fail-closed: malformed, duplicate and oversized Content-Length are
# fatal rather than leading to an unbounded allocation or protocol desynchronization.
if printf 'Content-Length: nope\r\n\r\n{}' | "${LSP}" >"${TMP}/bad_length.out" 2>"${TMP}/bad_length.err"; then
  echo "error: malformed Content-Length unexpectedly succeeded" >&2
  exit 1
fi
grep -Fq 'invalid Content-Length' "${TMP}/bad_length.err"

if printf 'Content-Length: 2\r\nContent-Length: 2\r\n\r\n{}' | "${LSP}" >"${TMP}/duplicate_length.out" 2>"${TMP}/duplicate_length.err"; then
  echo "error: duplicate Content-Length unexpectedly succeeded" >&2
  exit 1
fi
grep -Fq 'duplicate Content-Length' "${TMP}/duplicate_length.err"

if printf 'Content-Length: 1048577\r\n\r\n' | "${LSP}" >"${TMP}/oversized.out" 2>"${TMP}/oversized.err"; then
  echo "error: oversized LSP message unexpectedly succeeded" >&2
  exit 1
fi
grep -Fq 'bounded LSP message size' "${TMP}/oversized.err"

# The LSP must not report an empty diagnostic set when its compiler oracle is
# unavailable. This prevents editor tooling from manufacturing false success.
VALID_JSON='"int value;\n"'
MISSING_URI="file://${TMP}/oracle-missing.short"
{
  send_message '{"jsonrpc":"2.0","id":30,"method":"initialize","params":{}}'
  send_message "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"${MISSING_URI}\",\"languageId\":\"shorthand\",\"version\":1,\"text\":${VALID_JSON}}}}"
  send_message '{"jsonrpc":"2.0","id":31,"method":"shutdown","params":null}'
  send_message '{"jsonrpc":"2.0","method":"exit","params":null}'
} | SHORTHAND_COMPILER="${TMP}/definitely-missing-short-hand" timeout 8s "${LSP}" >"${TMP}/missing_oracle.out" 2>"${TMP}/missing_oracle.err"
grep -Fq '"code":"SHLSP900"' "${TMP}/missing_oracle.out"
grep -Fq 'compiler oracle unavailable' "${TMP}/missing_oracle.out"

# LSP lifecycle requires a non-zero exit when the client sends exit without a
# preceding shutdown request.
if {
  send_message '{"jsonrpc":"2.0","id":40,"method":"initialize","params":{}}'
  send_message '{"jsonrpc":"2.0","method":"exit","params":null}'
} | "${LSP}" >"${TMP}/early_exit.out" 2>"${TMP}/early_exit.err"; then
  echo "error: LSP exit without shutdown unexpectedly returned success" >&2
  exit 1
fi

printf 'PASS syntax highlighting LSP protocol compiler-diagnostics navigation cancellation UTF16 bounded-framing gate compiler=%s\n' "${CXX}"
