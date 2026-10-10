#!/usr/bin/env bash
set -euo pipefail

VERSION="1.30.0"
# Digests are pinned from the upstream v1.30.0 release asset metadata.
# https://github.com/microsoft/onnxruntime/releases/tag/v1.30.0
case "$(uname -s):$(uname -m)" in
  Linux:x86_64)
    PLATFORM=linux-x64
    SHA256=a5ed5a3cac51fbb2e90da632ae43d19212faaa20e76484e62bcb7c23ddb3b3fd ;;
  Linux:aarch64|Linux:arm64)
    PLATFORM=linux-aarch64
    SHA256=e16a27a8ed330bbc698df7330b0cf56e722f354e3bcc92118682c74ef3c3e3da ;;
  Darwin:arm64)
    PLATFORM=osx-arm64
    SHA256=6ebb5062a934537c352937821f9fe9718e7de1a2db1122a93dd363ffd53a7012 ;;
  MINGW*:x86_64|MSYS*:x86_64)
    PLATFORM=win-x64
    SHA256=c6ba983baf5681af108599675d2a89c2d145512d02de28aed0bff177cd0ba949 ;;
  *) echo "error: no pinned ONNX Runtime CPU SDK for this native platform" >&2; exit 1 ;;
esac
EXT=tgz
[[ "${PLATFORM}" != win-x64 ]] || EXT=zip
ARCHIVE="onnxruntime-${PLATFORM}-${VERSION}.${EXT}"
URL="https://github.com/microsoft/onnxruntime/releases/download/v${VERSION}/${ARCHIVE}"
DESTINATION="${1:-${RUNNER_TEMP:-/tmp}/shorthand-onnxruntime-${VERSION}}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT
for tool in curl cmake; do
  command -v "${tool}" >/dev/null 2>&1 || { echo "error: required SDK acquisition tool missing: ${tool}" >&2; exit 1; }
done

curl --fail --location --silent --show-error \
  --retry 3 --retry-all-errors --connect-timeout 20 --max-time 180 \
  "${URL}" -o "${WORK_DIR}/${ARCHIVE}"

actual="$(cmake -E sha256sum "${WORK_DIR}/${ARCHIVE}" | awk '{print $1}')"
if [[ "${actual}" != "${SHA256}" ]]; then
  echo "error: ONNX Runtime ${VERSION} asset checksum mismatch" >&2
  echo "expected=${SHA256}" >&2
  echo "actual=${actual}" >&2
  exit 1
fi

rm -rf "${DESTINATION}"
mkdir -p "${DESTINATION}"
(cd "${WORK_DIR}" && cmake -E tar xf "${ARCHIVE}")
cp -R "${WORK_DIR}/onnxruntime-${PLATFORM}-${VERSION}/." "${DESTINATION}/"

[[ -s "${DESTINATION}/include/onnxruntime_cxx_api.h" ]] || { echo "error: ONNX Runtime C++ header missing after verified extraction" >&2; exit 1; }
[[ -s "${DESTINATION}/lib/libonnxruntime.so" || -s "${DESTINATION}/lib/libonnxruntime.dylib" || -s "${DESTINATION}/lib/onnxruntime.dll" ]] || { echo "error: ONNX Runtime shared library missing after verified extraction" >&2; exit 1; }

printf 'ONNXRUNTIME_CI_ROOT=%s\n' "${DESTINATION}"
printf 'ONNXRUNTIME_CI_PLATFORM=%s\n' "${PLATFORM}"
printf 'ONNXRUNTIME_CI_VERSION=%s\n' "${VERSION}"
printf 'ONNXRUNTIME_CI_SHA256=%s\n' "${SHA256}"
printf 'PASS verified ONNX Runtime CPU qualification SDK acquisition\n'
