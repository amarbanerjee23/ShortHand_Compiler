#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP="${TMPDIR:-/tmp}/shorthand-release-bundle-test"
STAGE="${TMP}/stage"
OUT="${TMP}/out"
rm -rf "${TMP}"
mkdir -p "${STAGE}/bin" "${STAGE}/lib"
printf '#!/bin/sh\nexit 0\n' > "${STAGE}/bin/short_hand"
printf '#!/bin/sh\nexit 0\n' > "${STAGE}/bin/green_ai_tool"
chmod +x "${STAGE}/bin/short_hand" "${STAGE}/bin/green_ai_tool"
printf 'runtime-fixture\n' > "${STAGE}/lib/libshorthand_runtime.a"

bash "${ROOT_DIR}/scripts/prepare_release_bundle.sh" v1.0.0 linux-x64 "${STAGE}" "${OUT}" >/tmp/shorthand_prepare_release_bundle.out
bash "${ROOT_DIR}/scripts/verify_release_bundle.sh" "${OUT}" >/tmp/shorthand_verify_release_bundle.out

archive="$(find "${OUT}" -maxdepth 1 -name '*.tar' -print -quit)"
printf 'tamper\n' >> "${archive}"
if bash "${ROOT_DIR}/scripts/verify_release_bundle.sh" "${OUT}" >/tmp/shorthand_release_tamper_negative.out 2>&1; then
  echo "error: tampered release artifact unexpectedly verified" >&2
  exit 1
fi

rm -rf "${OUT}"
bash "${ROOT_DIR}/scripts/prepare_release_bundle.sh" v1.0.0 linux-x64 "${STAGE}" "${OUT}" >/tmp/shorthand_prepare_release_bundle_second.out
if bash "${ROOT_DIR}/scripts/verify_release_bundle.sh" "${OUT}" --publication >/tmp/shorthand_release_unsigned_negative.out 2>&1; then
  echo "error: unsigned candidate unexpectedly passed publication verification" >&2
  exit 1
fi

# Native Windows CMake uses drive-letter paths and CRLF output even when called
# from MSYS2 Bash. The published checksum file must still use portable basenames.
mkdir -p "${TMP}/native-tools"
cat > "${TMP}/native-tools/cmake" <<'SH'
#!/usr/bin/env bash
set -euo pipefail
if [[ "${1:-}" == -E && "${2:-}" == sha256sum ]]; then
  shift 2
  for subject in "$@"; do
    digest="$("${SHORTHAND_TEST_REAL_CMAKE}" -E sha256sum "${subject}" | awk '{print $1}')"
    native_path="${subject}"
    if [[ "${subject}" == /* ]]; then native_path="C:${subject}"; fi
    printf '%s  %s\r\n' "${digest}" "${native_path}"
  done
else
  exec "${SHORTHAND_TEST_REAL_CMAKE}" "$@"
fi
SH
chmod +x "${TMP}/native-tools/cmake"
cp "${STAGE}/bin/short_hand" "${STAGE}/bin/short_hand.exe"
cp "${STAGE}/bin/green_ai_tool" "${STAGE}/bin/green_ai_tool.exe"
export SHORTHAND_TEST_REAL_CMAKE="$(command -v cmake)"
export PATH="${TMP}/native-tools:${PATH}"
NATIVE_OUT="${TMP}/native output [checksum]"
bash "${ROOT_DIR}/scripts/prepare_release_bundle.sh" v1.0.0 windows-x64 "${STAGE}" "${NATIVE_OUT}" >/tmp/shorthand_prepare_release_native.out
bash "${ROOT_DIR}/scripts/verify_release_bundle.sh" "${NATIVE_OUT}" >/tmp/shorthand_verify_release_native.out

printf 'PASS release bundle checksum tamper, unsigned-publication and native Windows checksum controls\n'
