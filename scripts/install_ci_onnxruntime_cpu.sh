#!/usr/bin/env bash
set -euo pipefail

VERSION="1.30.0"
# Digests are pinned from the upstream v1.30.0 release asset metadata.
# https://github.com/microsoft/onnxruntime/releases/tag/v1.30.0
# Official Microsoft-signed NuGet fallback package for the same 1.30.0 release:
# https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime/1.30.0/microsoft.ml.onnxruntime.1.30.0.nupkg
NUGET_SHA256="30df119bd9164eaa187666d93147448fdd4a9a1a3caf83f1505daf7af2d1c5d2"

case "$(uname -s):$(uname -m)" in
  Linux:x86_64)
    PLATFORM=linux-x64
    NUGET_RID=linux-x64
    SHA256=a5ed5a3cac51fbb2e90da632ae43d19212faaa20e76484e62bcb7c23ddb3b3fd ;;
  Linux:aarch64|Linux:arm64)
    PLATFORM=linux-aarch64
    NUGET_RID=linux-arm64
    SHA256=e16a27a8ed330bbc698df7330b0cf56e722f354e3bcc92118682c74ef3c3e3da ;;
  Darwin:arm64)
    PLATFORM=osx-arm64
    NUGET_RID=osx-arm64
    SHA256=6ebb5062a934537c352937821f9fe9718e7de1a2db1122a93dd363ffd53a7012 ;;
  MINGW*:x86_64|MSYS*:x86_64)
    PLATFORM=win-x64
    NUGET_RID=win-x64
    SHA256=c6ba983baf5681af108599675d2a89c2d145512d02de28aed0bff177cd0ba949 ;;
  *) echo "error: no pinned ONNX Runtime CPU SDK for this native platform" >&2; exit 1 ;;
esac
EXT=tgz
[[ "${PLATFORM}" != win-x64 ]] || EXT=zip
ARCHIVE="onnxruntime-${PLATFORM}-${VERSION}.${EXT}"
URL="https://github.com/microsoft/onnxruntime/releases/download/v${VERSION}/${ARCHIVE}"
NUGET_ARCHIVE="microsoft.ml.onnxruntime.${VERSION}.nupkg"
NUGET_URL="https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime/${VERSION}/${NUGET_ARCHIVE}"
DESTINATION="${1:-${RUNNER_TEMP:-/tmp}/shorthand-onnxruntime-${VERSION}}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT
for tool in curl cmake; do
  command -v "${tool}" >/dev/null 2>&1 || { echo "error: required SDK acquisition tool missing: ${tool}" >&2; exit 1; }
done

SOURCE_SHA256="${SHA256}"
if curl --fail --location --silent --show-error \
  --retry 1 --connect-timeout 15 --max-time 180 \
  "${URL}" -o "${WORK_DIR}/${ARCHIVE}"; then
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
else
  echo "warning: primary ONNX Runtime archive unavailable (${URL}); falling back to pinned NuGet package" >&2
  curl --fail --location --silent --show-error \
    --retry 3 --retry-all-errors --connect-timeout 20 --max-time 300 \
    "${NUGET_URL}" -o "${WORK_DIR}/${NUGET_ARCHIVE}"
  actual_nuget="$(cmake -E sha256sum "${WORK_DIR}/${NUGET_ARCHIVE}" | awk '{print $1}')"
  if [[ "${actual_nuget}" != "${NUGET_SHA256}" ]]; then
    echo "error: ONNX Runtime ${VERSION} NuGet package checksum mismatch" >&2
    echo "expected=${NUGET_SHA256}" >&2
    echo "actual=${actual_nuget}" >&2
    exit 1
  fi
  SOURCE_SHA256="${NUGET_SHA256}"

  mkdir -p "${WORK_DIR}/nuget"
  (cd "${WORK_DIR}/nuget" && cmake -E tar xf "../${NUGET_ARCHIVE}")
  nuspec="${WORK_DIR}/nuget/Microsoft.ML.OnnxRuntime.nuspec"
  [[ -s "${nuspec}" ]] || { echo "error: NuGet package missing nuspec" >&2; exit 1; }
  grep -Fq "<id>Microsoft.ML.OnnxRuntime</id>" "${nuspec}" || { echo "error: unexpected NuGet package id" >&2; exit 1; }
  grep -Fq "<version>${VERSION}</version>" "${nuspec}" || { echo "error: unexpected NuGet package version" >&2; exit 1; }

  rm -rf "${DESTINATION}"
  mkdir -p "${DESTINATION}/include" "${DESTINATION}/lib"
  cp -R "${WORK_DIR}/nuget/build/native/include/." "${DESTINATION}/include/"
  cp "${WORK_DIR}/nuget/LICENSE" "${WORK_DIR}/nuget/ThirdPartyNotices.txt" "${DESTINATION}/"
  for doc in Privacy.md README.md; do
    [[ ! -f "${WORK_DIR}/nuget/${doc}" ]] || cp "${WORK_DIR}/nuget/${doc}" "${DESTINATION}/"
  done
  printf '%s\n' "${VERSION}" > "${DESTINATION}/VERSION_NUMBER"

  case "${PLATFORM}" in
    linux-x64|linux-aarch64)
      cp "${WORK_DIR}/nuget/runtimes/${NUGET_RID}/native/libonnxruntime.so" "${DESTINATION}/lib/libonnxruntime.so.${VERSION}"
      chmod 0755 "${DESTINATION}/lib/libonnxruntime.so.${VERSION}"
      ln -sf "libonnxruntime.so.${VERSION}" "${DESTINATION}/lib/libonnxruntime.so.1"
      ln -sf "libonnxruntime.so.1" "${DESTINATION}/lib/libonnxruntime.so"
      if [[ -f "${WORK_DIR}/nuget/runtimes/${NUGET_RID}/native/libonnxruntime_providers_shared.so" ]]; then
        cp "${WORK_DIR}/nuget/runtimes/${NUGET_RID}/native/libonnxruntime_providers_shared.so" "${DESTINATION}/lib/libonnxruntime_providers_shared.so"
        chmod 0755 "${DESTINATION}/lib/libonnxruntime_providers_shared.so"
      fi ;;
    osx-arm64)
      cp "${WORK_DIR}/nuget/runtimes/${NUGET_RID}/native/libonnxruntime.dylib" "${DESTINATION}/lib/libonnxruntime.${VERSION}.dylib"
      chmod 0755 "${DESTINATION}/lib/libonnxruntime.${VERSION}.dylib"
      ln -sf "libonnxruntime.${VERSION}.dylib" "${DESTINATION}/lib/libonnxruntime.1.dylib"
      ln -sf "libonnxruntime.1.dylib" "${DESTINATION}/lib/libonnxruntime.dylib" ;;
    win-x64)
      cp "${WORK_DIR}/nuget/runtimes/${NUGET_RID}/native/"* "${DESTINATION}/lib/"
      chmod 0755 "${DESTINATION}/lib/"*.dll ;;
  esac
fi

[[ -s "${DESTINATION}/include/onnxruntime_cxx_api.h" ]] || { echo "error: ONNX Runtime C++ header missing after verified extraction" >&2; exit 1; }
[[ -s "${DESTINATION}/LICENSE" && -s "${DESTINATION}/ThirdPartyNotices.txt" && -s "${DESTINATION}/VERSION_NUMBER" ]] || { echo "error: ONNX Runtime redistribution notice missing after verified extraction" >&2; exit 1; }
[[ -s "${DESTINATION}/lib/libonnxruntime.so" || -s "${DESTINATION}/lib/libonnxruntime.dylib" || -s "${DESTINATION}/lib/onnxruntime.dll" ]] || { echo "error: ONNX Runtime shared library missing after verified extraction" >&2; exit 1; }

printf 'ONNXRUNTIME_CI_ROOT=%s\n' "${DESTINATION}"
printf 'ONNXRUNTIME_CI_PLATFORM=%s\n' "${PLATFORM}"
printf 'ONNXRUNTIME_CI_VERSION=%s\n' "${VERSION}"
printf 'ONNXRUNTIME_CI_SHA256=%s\n' "${SOURCE_SHA256}"
printf 'PASS verified ONNX Runtime CPU qualification SDK acquisition\n'
