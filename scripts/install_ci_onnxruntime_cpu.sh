#!/usr/bin/env bash
set -euo pipefail

VERSION="1.20.1"
ARCHIVE="onnxruntime-linux-x64-${VERSION}.tgz"
URL="https://github.com/microsoft/onnxruntime/releases/download/v${VERSION}/${ARCHIVE}"
SHA256="b6179dfb9ec297862daa3f30691942fe994441c63df3ff3c1fa94b022bba20d8"
DESTINATION="${1:-${RUNNER_TEMP:-/tmp}/shorthand-onnxruntime-${VERSION}}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

if [[ "$(uname -s)" != "Linux" || "$(uname -m)" != "x86_64" ]]; then
  echo "error: mandatory ONNX Runtime CPU qualification asset is scoped to Linux x86_64" >&2
  exit 1
fi

for tool in curl sha256sum tar; do
  command -v "${tool}" >/dev/null 2>&1 || { echo "error: required SDK acquisition tool missing: ${tool}" >&2; exit 1; }
done

curl --fail --location --silent --show-error \
  --retry 3 --retry-all-errors --connect-timeout 20 --max-time 180 \
  "${URL}" -o "${WORK_DIR}/${ARCHIVE}"

actual="$(sha256sum "${WORK_DIR}/${ARCHIVE}" | awk '{print $1}')"
if [[ "${actual}" != "${SHA256}" ]]; then
  echo "error: ONNX Runtime ${VERSION} asset checksum mismatch" >&2
  echo "expected=${SHA256}" >&2
  echo "actual=${actual}" >&2
  exit 1
fi

rm -rf "${DESTINATION}"
mkdir -p "${DESTINATION}"
tar -xzf "${WORK_DIR}/${ARCHIVE}" -C "${DESTINATION}" --strip-components=1

[[ -s "${DESTINATION}/include/onnxruntime_cxx_api.h" ]] || { echo "error: ONNX Runtime C++ header missing after verified extraction" >&2; exit 1; }
[[ -s "${DESTINATION}/lib/libonnxruntime.so" || -s "${DESTINATION}/lib/libonnxruntime.so.${VERSION}" ]] || { echo "error: ONNX Runtime shared library missing after verified extraction" >&2; exit 1; }

printf 'ONNXRUNTIME_CI_ROOT=%s\n' "${DESTINATION}"
printf 'ONNXRUNTIME_CI_VERSION=%s\n' "${VERSION}"
printf 'ONNXRUNTIME_CI_SHA256=%s\n' "${SHA256}"
printf 'PASS verified ONNX Runtime CPU qualification SDK acquisition\n'
