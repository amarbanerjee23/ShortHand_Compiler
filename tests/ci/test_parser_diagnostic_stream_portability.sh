#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MAIN="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src/main.cpp"

[[ -s "${MAIN}" ]] || { echo "error: compiler main source missing" >&2; exit 1; }

grep -Fq 'flex_output = tmpfile();' "${MAIN}" || {
  echo "error: Flex diagnostic stream must use portable tmpfile()" >&2
  exit 1
}
grep -Fq 'bison_output = tmpfile();' "${MAIN}" || {
  echo "error: Bison diagnostic stream must use portable tmpfile()" >&2
  exit 1
}

if grep -Fq 'fopen("/dev/null"' "${MAIN}"; then
  echo "error: compiler entry point reintroduced POSIX-only /dev/null diagnostic streams" >&2
  exit 1
fi

# Both temporary streams remain owned by cleanup_parser_resources().
[[ "$(grep -Fc 'fclose(flex_output);' "${MAIN}")" -eq 1 ]] || {
  echo "error: Flex temporary stream cleanup contract changed" >&2
  exit 1
}
[[ "$(grep -Fc 'fclose(bison_output);' "${MAIN}")" -eq 1 ]] || {
  echo "error: Bison temporary stream cleanup contract changed" >&2
  exit 1
}

echo "PASS portable parser diagnostic stream regression"
