#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
[[ "${MSYSTEM:-}" == UCRT64 ]] || { echo 'error: pinned Windows LLVM requires MSYS2 UCRT64' >&2; exit 1; }
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT
archives=()
packages=()

# Keep the compiler and its exact-version dependencies together. The rolling
# repository must not silently promote the toolchain used to qualify releases.
while read -r digest name; do
  [[ "${digest}" =~ ^[0-9a-f]{64}$ && "${name}" =~ ^mingw-w64-ucrt-x86_64-(clang|clang-libs|llvm|llvm-libs|llvm-tools|lld)-22\.1\.8-3-any\.pkg\.tar\.zst$ ]] || {
    echo 'error: invalid pinned Windows LLVM manifest entry' >&2; exit 1;
  }
  archive="${WORK_DIR}/${name}"
  for suffix in '' '.sig'; do
    curl --fail --location --silent --show-error --retry 3 --retry-all-errors \
      --connect-timeout 20 --max-time 180 \
      "https://repo.msys2.org/mingw/ucrt64/${name}${suffix}" -o "${archive}${suffix}"
  done
  printf '%s  %s\n' "${digest}" "${archive}" | sha256sum --check --strict -
  pacman-key --verify "${archive}.sig" "${archive}"
  archives+=("${archive}")
  packages+=("${name%-22.1.8-3-any.pkg.tar.zst}")
done < "${ROOT_DIR}/scripts/windows_llvm22.SHA256SUMS"
[[ "${#archives[@]}" == 6 ]] || { echo 'error: Windows LLVM pin must contain all six packages' >&2; exit 1; }

pacman -U --noconfirm "${archives[@]}"
for package in "${packages[@]}"; do
  [[ "$(pacman -Q "${package}")" == "${package} 22.1.8-3" ]] || {
    echo "error: pinned Windows LLVM package version mismatch: ${package}" >&2; exit 1;
  }
done
[[ "$(/ucrt64/bin/llvm-config --version | tr -d '\r')" == 22.1.8 ]] || {
  echo 'error: installed Windows LLVM version mismatch' >&2; exit 1;
}
printf 'PASS pinned Windows UCRT64 LLVM toolchain version=22.1.8-3 packages=6\n'
