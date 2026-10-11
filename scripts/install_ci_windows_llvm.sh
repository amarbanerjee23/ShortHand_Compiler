#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
[[ "${MSYSTEM:-}" == UCRT64 ]] || { echo 'error: pinned Windows LLVM requires MSYS2 UCRT64' >&2; exit 1; }
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT
WITH_MLIR="${1:-}"
[[ -z "${WITH_MLIR}" || "${WITH_MLIR}" == --with-mlir ]] || { echo "usage: $0 [--with-mlir]" >&2; exit 2; }
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
  archives+=("${archive}")
  packages+=("${name%-22.1.8-3-any.pkg.tar.zst}")
done < "${ROOT_DIR}/scripts/windows_llvm22.SHA256SUMS"
[[ "${#archives[@]}" == 6 ]] || { echo 'error: Windows LLVM pin must contain all six packages' >&2; exit 1; }

if [[ "${WITH_MLIR}" == --with-mlir ]]; then
  name=mingw-w64-ucrt-x86_64-mlir-22.1.8-1-any.pkg.tar.zst
  archive="${WORK_DIR}/${name}"
  for suffix in '' '.sig'; do
    curl --fail --location --silent --show-error --retry 3 --retry-all-errors \
      --connect-timeout 20 --max-time 300 \
      "https://repo.msys2.org/mingw/ucrt64/${name}${suffix}" -o "${archive}${suffix}"
  done
  printf '%s  %s\n' 5274ca8a4c0483f62bc63a2b353987b5aee6686abea3e6be747816e79b15cc4b "${archive}" | sha256sum --check --strict -
  archives+=("${archive}")
fi

# Let the package manager verify signatures with its existing trusted keyring.
# Local packages default to optional signatures; require trusted signatures for
# this transaction without changing the runner's global pacman configuration.
PACMAN_CONFIG="${WORK_DIR}/pacman.conf"
sed -E 's/^[[:space:]]*LocalFileSigLevel[[:space:]]*=.*/LocalFileSigLevel = Required TrustedOnly/' \
  /etc/pacman.conf > "${PACMAN_CONFIG}"
grep -qx 'LocalFileSigLevel = Required TrustedOnly' "${PACMAN_CONFIG}" || {
  echo 'error: cannot enforce trusted signatures for pinned Windows LLVM' >&2; exit 1;
}
pacman --config "${PACMAN_CONFIG}" -U --noconfirm "${archives[@]}"
for package in "${packages[@]}"; do
  [[ "$(pacman -Q "${package}")" == "${package} 22.1.8-3" ]] || {
    echo "error: pinned Windows LLVM package version mismatch: ${package}" >&2; exit 1;
  }
done
[[ "$(/ucrt64/bin/llvm-config --version | tr -d '\r')" == 22.1.8 ]] || {
  echo 'error: installed Windows LLVM version mismatch' >&2; exit 1;
}
printf 'PASS pinned Windows UCRT64 LLVM toolchain version=22.1.8-3 packages=6\n'
if [[ "${WITH_MLIR}" == --with-mlir ]]; then
  [[ "$(pacman -Q mingw-w64-ucrt-x86_64-mlir)" == "mingw-w64-ucrt-x86_64-mlir 22.1.8-1" ]]
  printf 'PASS pinned Windows MLIR version=22.1.8-1\n'
fi
