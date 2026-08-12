#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_ROOT="${SHORTHAND_REPRO_WORK_ROOT:-${TMPDIR:-/tmp}/shorthand-reproducibility}"
ARTIFACT="${SHORTHAND_REPRO_ARTIFACT:-/tmp/shorthand_reproducibility.json}"
LOGICAL_PREFIX="/opt/shorthand"
SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-1704067200}"
export SOURCE_DATE_EPOCH LC_ALL=C TZ=UTC

for tool in cmake ninja sha256sum find sort cmp c++; do
  command -v "${tool}" >/dev/null 2>&1 || { echo "error: reproducibility gate requires ${tool}" >&2; exit 1; }
done
command -v llvm-config >/dev/null 2>&1 || { echo "error: reproducibility gate requires llvm-config" >&2; exit 1; }

rm -rf "${WORK_ROOT}"
mkdir -p "${WORK_ROOT}"

build_once() {
  local label="$1"
  local build_dir="${WORK_ROOT}/${label}/build"
  local stage_dir="${WORK_ROOT}/${label}/stage"
  mkdir -p "${build_dir}" "${stage_dir}"

  local prefix_flags
  prefix_flags="-ffile-prefix-map=${ROOT_DIR}=/usr/src/shorthand -fdebug-prefix-map=${ROOT_DIR}=/usr/src/shorthand -ffile-prefix-map=${build_dir}=/usr/src/shorthand/build -fdebug-prefix-map=${build_dir}=/usr/src/shorthand/build"

  cmake -S "${ROOT_DIR}" -B "${build_dir}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${LOGICAL_PREFIX}" \
    -DCMAKE_CXX_FLAGS="${prefix_flags}" \
    -DSHORTHAND_BUILD_TESTING=OFF \
    >"${WORK_ROOT}/${label}/configure.log" 2>&1
  cmake --build "${build_dir}" --parallel 2 \
    >"${WORK_ROOT}/${label}/build.log" 2>&1
  DESTDIR="${stage_dir}" cmake --install "${build_dir}" \
    >"${WORK_ROOT}/${label}/install.log" 2>&1

  [[ -x "${build_dir}/short_hand" ]] || { echo "error: ${label} compiler artifact missing" >&2; exit 1; }
  [[ -x "${build_dir}/green_ai_tool" ]] || { echo "error: ${label} Green AI tool artifact missing" >&2; exit 1; }

  (
    cd "${stage_dir}${LOGICAL_PREFIX}"
    find . -type f -print0 | sort -z | xargs -0 sha256sum
  ) >"${WORK_ROOT}/${label}/install.sha256"

  sha256sum "${build_dir}/short_hand" | awk '{print $1 "  build/short_hand"}' \
    >"${WORK_ROOT}/${label}/build.sha256"
  sha256sum "${build_dir}/green_ai_tool" | awk '{print $1 "  build/green_ai_tool"}' \
    >>"${WORK_ROOT}/${label}/build.sha256"
}

build_once a
build_once b

if ! cmp -s "${WORK_ROOT}/a/install.sha256" "${WORK_ROOT}/b/install.sha256"; then
  echo "error: independently installed artifacts are not byte-for-byte reproducible" >&2
  diff -u "${WORK_ROOT}/a/install.sha256" "${WORK_ROOT}/b/install.sha256" >&2 || true
  exit 1
fi
if ! cmp -s "${WORK_ROOT}/a/build.sha256" "${WORK_ROOT}/b/build.sha256"; then
  echo "error: compiler/tool build artifacts are not byte-for-byte reproducible" >&2
  diff -u "${WORK_ROOT}/a/build.sha256" "${WORK_ROOT}/b/build.sha256" >&2 || true
  exit 1
fi

# Negative self-test: the detector must reject an intentionally modified manifest.
cp "${WORK_ROOT}/a/install.sha256" "${WORK_ROOT}/tampered.sha256"
printf '# tamper sentinel\n' >> "${WORK_ROOT}/tampered.sha256"
if cmp -s "${WORK_ROOT}/a/install.sha256" "${WORK_ROOT}/tampered.sha256"; then
  echo "error: reproducibility tamper self-test did not detect a modified manifest" >&2
  exit 1
fi

manifest_hash="$(sha256sum "${WORK_ROOT}/a/install.sha256" | awk '{print $1}')"
compiler_hash="$(awk '$2 == "build/short_hand" {print $1}' "${WORK_ROOT}/a/build.sha256")"
green_hash="$(awk '$2 == "build/green_ai_tool" {print $1}' "${WORK_ROOT}/a/build.sha256")"
llvm_version="$(llvm-config --version | head -n1)"
cxx_version="$(c++ --version | head -n1 | sed 's/"/\\"/g')"
architecture="$(uname -m)"

cat >"${ARTIFACT}" <<JSON
{"schema":"shorthand.reproducible.build.v1","status":"pass","source_date_epoch":"${SOURCE_DATE_EPOCH}","architecture":"${architecture}","llvm":"${llvm_version}","cxx":"${cxx_version}","install_manifest_sha256":"${manifest_hash}","short_hand_sha256":"${compiler_hash}","green_ai_tool_sha256":"${green_hash}"}
JSON

echo "PASS clean reproducible build gate manifest=${manifest_hash} compiler=${compiler_hash}"
