#!/usr/bin/env bash
set -euo pipefail

# ShortHand one-shot build infrastructure setup.
#
# This script installs/builds the local development toolchain needed for the
# ShortHand compiler and optional native C++ AI backends. It intentionally does
# not vendor large third-party SDK archives into git. Instead, it downloads or
# detects them under ./third_party and writes ./shorthand_env.sh.
#
# Core installed/detected components:
#   - LLVM/Clang/llc/llvm-config
#   - Flex/Bison/libfl
#   - C++ compiler/build tools
#   - ONNX Runtime C++ SDK, optional but downloaded by default
#   - LibTorch C++ SDK, optional; downloaded when DOWNLOAD_LIBTORCH=1
#   - TensorRT, optional; detected if already installed or configured
#
# Usage:
#   bash setup_build_infra.sh
#   source ./shorthand_env.sh
#   bash scripts/smoke_test.sh
#
# Useful environment switches:
#   LLVM_VERSION=18 bash setup_build_infra.sh
#   DOWNLOAD_ONNXRUNTIME=0 bash setup_build_infra.sh
#   DOWNLOAD_LIBTORCH=1 bash setup_build_infra.sh
#   LIBTORCH_URL=https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcpu.zip bash setup_build_infra.sh
#   TENSORRT_ROOT=/path/to/TensorRT bash setup_build_infra.sh

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
THIRD_PARTY_DIR="${ROOT_DIR}/third_party"
DOWNLOAD_DIR="${THIRD_PARTY_DIR}/downloads"
ENV_FILE="${ROOT_DIR}/shorthand_env.sh"

LLVM_VERSION="${LLVM_VERSION:-}"
DOWNLOAD_ONNXRUNTIME="${DOWNLOAD_ONNXRUNTIME:-1}"
DOWNLOAD_LIBTORCH="${DOWNLOAD_LIBTORCH:-0}"
ONNXRUNTIME_VERSION="${ONNXRUNTIME_VERSION:-latest}"
LIBTORCH_URL="${LIBTORCH_URL:-}"
TENSORRT_ROOT="${TENSORRT_ROOT:-}"
RUN_SMOKE_TEST="${RUN_SMOKE_TEST:-1}"

mkdir -p "${THIRD_PARTY_DIR}" "${DOWNLOAD_DIR}" "${BUILD_DIR}"

log() { printf '\n[setup] %s\n' "$*"; }
warn() { printf '\n[setup][warning] %s\n' "$*" >&2; }
fail() { printf '\n[setup][error] %s\n' "$*" >&2; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }

os_name="$(uname -s)"
arch_name="$(uname -m)"

need_sudo=""
if [[ "${EUID:-$(id -u)}" -ne 0 ]] && have sudo; then
  need_sudo="sudo"
fi

install_linux_deps() {
  if have apt-get; then
    log "Installing Ubuntu/Debian build dependencies"
    ${need_sudo} apt-get update
    if [[ -n "${LLVM_VERSION}" ]]; then
      ${need_sudo} apt-get install -y \
        build-essential cmake ninja-build curl ca-certificates unzip tar pkg-config \
        flex bison libfl-dev \
        "llvm-${LLVM_VERSION}" "llvm-${LLVM_VERSION}-dev" "clang-${LLVM_VERSION}"
    else
      ${need_sudo} apt-get install -y \
        build-essential cmake ninja-build curl ca-certificates unzip tar pkg-config \
        flex bison libfl-dev \
        llvm llvm-dev clang
    fi
  elif have dnf; then
    log "Installing Fedora/RHEL build dependencies"
    ${need_sudo} dnf install -y gcc gcc-c++ make cmake ninja-build curl unzip tar pkgconfig flex bison llvm llvm-devel clang
  elif have yum; then
    log "Installing RHEL/CentOS build dependencies"
    ${need_sudo} yum install -y gcc gcc-c++ make cmake ninja-build curl unzip tar pkgconfig flex bison llvm llvm-devel clang
  else
    warn "No supported Linux package manager found. Install LLVM, Clang, Flex, Bison, libfl, make, and g++ manually."
  fi
}

install_macos_deps() {
  if ! have brew; then
    warn "Homebrew not found. Install LLVM, Flex, Bison, make, and g++ manually or install Homebrew first."
    return
  fi
  log "Installing macOS build dependencies with Homebrew"
  brew install llvm flex bison cmake ninja curl || true
}

resolve_llvm_config() {
  local candidate=""
  if [[ -n "${LLVM_VERSION}" ]] && have "llvm-config-${LLVM_VERSION}"; then
    candidate="$(command -v "llvm-config-${LLVM_VERSION}")"
  elif have llvm-config; then
    candidate="$(command -v llvm-config)"
  elif [[ -x /opt/homebrew/opt/llvm/bin/llvm-config ]]; then
    candidate="/opt/homebrew/opt/llvm/bin/llvm-config"
  elif [[ -x /usr/local/opt/llvm/bin/llvm-config ]]; then
    candidate="/usr/local/opt/llvm/bin/llvm-config"
  fi

  [[ -n "${candidate}" ]] || fail "llvm-config not found after dependency installation."
  echo "${candidate}"
}

resolve_clang() {
  if [[ -n "${LLVM_VERSION}" ]] && have "clang-${LLVM_VERSION}"; then
    command -v "clang-${LLVM_VERSION}"
  elif have clang; then
    command -v clang
  elif [[ -x /opt/homebrew/opt/llvm/bin/clang ]]; then
    echo "/opt/homebrew/opt/llvm/bin/clang"
  elif [[ -x /usr/local/opt/llvm/bin/clang ]]; then
    echo "/usr/local/opt/llvm/bin/clang"
  else
    echo "clang"
  fi
}

resolve_llc() {
  if [[ -n "${LLVM_VERSION}" ]] && have "llc-${LLVM_VERSION}"; then
    command -v "llc-${LLVM_VERSION}"
  elif have llc; then
    command -v llc
  elif [[ -x /opt/homebrew/opt/llvm/bin/llc ]]; then
    echo "/opt/homebrew/opt/llvm/bin/llc"
  elif [[ -x /usr/local/opt/llvm/bin/llc ]]; then
    echo "/usr/local/opt/llvm/bin/llc"
  else
    echo "llc"
  fi
}

latest_onnxruntime_version() {
  local api="https://api.github.com/repos/microsoft/onnxruntime/releases/latest"
  curl -fsSL "${api}" | sed -n 's/.*"tag_name": *"v\([^"]*\)".*/\1/p' | head -n 1
}

onnxruntime_asset_name() {
  case "${os_name}-${arch_name}" in
    Linux-x86_64) echo "onnxruntime-linux-x64" ;;
    Linux-aarch64|Linux-arm64) echo "onnxruntime-linux-aarch64" ;;
    Darwin-x86_64) echo "onnxruntime-osx-x86_64" ;;
    Darwin-arm64) echo "onnxruntime-osx-arm64" ;;
    *) return 1 ;;
  esac
}

download_onnxruntime() {
  [[ "${DOWNLOAD_ONNXRUNTIME}" == "1" ]] || return 0
  have curl || fail "curl is required to download ONNX Runtime."
  have tar || fail "tar is required to extract ONNX Runtime."

  local version="${ONNXRUNTIME_VERSION}"
  if [[ "${version}" == "latest" ]]; then
    log "Resolving latest ONNX Runtime release"
    version="$(latest_onnxruntime_version)"
  fi
  [[ -n "${version}" ]] || fail "Could not resolve ONNX Runtime version. Set ONNXRUNTIME_VERSION manually."

  local asset_prefix
  asset_prefix="$(onnxruntime_asset_name)" || { warn "Unsupported ONNX Runtime platform ${os_name}-${arch_name}; skipping."; return 0; }

  local archive="${asset_prefix}-${version}.tgz"
  local url="https://github.com/microsoft/onnxruntime/releases/download/v${version}/${archive}"
  local dest="${DOWNLOAD_DIR}/${archive}"
  local extract_dir="${THIRD_PARTY_DIR}/onnxruntime-${version}"

  if [[ -d "${extract_dir}" ]]; then
    log "ONNX Runtime already present at ${extract_dir}"
  else
    log "Downloading ONNX Runtime ${version}"
    curl -fL "${url}" -o "${dest}"
    rm -rf "${extract_dir}"
    mkdir -p "${extract_dir}"
    tar -xzf "${dest}" -C "${extract_dir}" --strip-components=1
  fi

  ln -sfn "${extract_dir}" "${THIRD_PARTY_DIR}/onnxruntime"
}

download_libtorch() {
  [[ "${DOWNLOAD_LIBTORCH}" == "1" ]] || return 0
  have curl || fail "curl is required to download LibTorch."
  have unzip || fail "unzip is required to extract LibTorch."

  local url="${LIBTORCH_URL}"
  if [[ -z "${url}" ]]; then
    url="https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcpu.zip"
    warn "LIBTORCH_URL not set; using default CPU LibTorch URL: ${url}"
  fi

  local archive="${DOWNLOAD_DIR}/libtorch.zip"
  local extract_dir="${THIRD_PARTY_DIR}/libtorch"
  if [[ -d "${extract_dir}" ]]; then
    log "LibTorch already present at ${extract_dir}"
  else
    log "Downloading LibTorch C++ SDK"
    curl -fL "${url}" -o "${archive}"
    rm -rf "${THIRD_PARTY_DIR}/libtorch" "${THIRD_PARTY_DIR}/libtorch_tmp"
    mkdir -p "${THIRD_PARTY_DIR}/libtorch_tmp"
    unzip -q "${archive}" -d "${THIRD_PARTY_DIR}/libtorch_tmp"
    if [[ -d "${THIRD_PARTY_DIR}/libtorch_tmp/libtorch" ]]; then
      mv "${THIRD_PARTY_DIR}/libtorch_tmp/libtorch" "${extract_dir}"
    else
      fail "LibTorch archive layout not recognized."
    fi
    rm -rf "${THIRD_PARTY_DIR}/libtorch_tmp"
  fi
}

detect_tensorrt() {
  if [[ -n "${TENSORRT_ROOT}" && -d "${TENSORRT_ROOT}" ]]; then
    log "Using TensorRT from TENSORRT_ROOT=${TENSORRT_ROOT}"
    return 0
  fi

  for d in /usr/local/TensorRT /usr/lib/x86_64-linux-gnu /usr/include/x86_64-linux-gnu /usr/src/tensorrt; do
    if [[ -e "${d}" ]]; then
      TENSORRT_ROOT="${d}"
      log "Detected possible TensorRT location: ${TENSORRT_ROOT}"
      return 0
    fi
  done

  warn "TensorRT was not downloaded automatically. NVIDIA TensorRT normally requires platform-specific packages and license/account acceptance. Set TENSORRT_ROOT after installing it from NVIDIA if needed."
}

write_env_file() {
  local llvm_config clang_bin llc_bin
  llvm_config="$(resolve_llvm_config)"
  clang_bin="$(resolve_clang)"
  llc_bin="$(resolve_llc)"

  local onnx_root=""
  if [[ -d "${THIRD_PARTY_DIR}/onnxruntime" ]]; then
    onnx_root="${THIRD_PARTY_DIR}/onnxruntime"
  fi

  local libtorch_root=""
  if [[ -d "${THIRD_PARTY_DIR}/libtorch" ]]; then
    libtorch_root="${THIRD_PARTY_DIR}/libtorch"
  fi

  cat > "${ENV_FILE}" <<EOF
# Generated by setup_build_infra.sh. Source this file before building optional AI targets.
export SHORT_HAND_ROOT="${ROOT_DIR}"
export SHORT_HAND_THIRD_PARTY="${THIRD_PARTY_DIR}"
export LLVM_CONFIG="${llvm_config}"
export CLANG_BIN="${clang_bin}"
export LLC_BIN="${llc_bin}"
export ONNXRUNTIME_ROOT="${onnx_root}"
export LIBTORCH_ROOT="${libtorch_root}"
export TENSORRT_ROOT="${TENSORRT_ROOT}"
export PATH="$(dirname "${clang_bin}"):$(dirname "${llc_bin}"):\${PATH}"
EOF
  log "Wrote ${ENV_FILE}"
}

build_and_verify() {
  # shellcheck source=/dev/null
  source "${ENV_FILE}"

  log "Toolchain summary"
  echo "LLVM_CONFIG=${LLVM_CONFIG}"
  "${LLVM_CONFIG}" --version
  "${CLANG_BIN}" --version | head -n 1 || true
  "${LLC_BIN}" --version | head -n 1 || true
  echo "ONNXRUNTIME_ROOT=${ONNXRUNTIME_ROOT}"
  echo "LIBTORCH_ROOT=${LIBTORCH_ROOT}"
  echo "TENSORRT_ROOT=${TENSORRT_ROOT}"

  log "Building GreenAI C++ tool"
  make -C "${SRC_DIR}" green_ai_tool

  log "Running GreenAI tests"
  make -C "${SRC_DIR}" test-green

  log "Building ShortHand compiler"
  make -C "${SRC_DIR}" clean all LLVM_CONFIG="${LLVM_CONFIG}"

  log "Building optional AI runtime binaries"
  if [[ -n "${ONNXRUNTIME_ROOT}" ]]; then
    make -C "${SRC_DIR}" ai_app ONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}"
  else
    make -C "${SRC_DIR}" ai_app
  fi

  if [[ -n "${LIBTORCH_ROOT}" ]]; then
    make -C "${SRC_DIR}" ai_train LIBTORCH_ROOT="${LIBTORCH_ROOT}"
  else
    make -C "${SRC_DIR}" ai_train
  fi

  if [[ "${RUN_SMOKE_TEST}" == "1" ]]; then
    log "Running repository smoke test"
    bash "${ROOT_DIR}/scripts/smoke_test.sh"
  fi

  log "Setup and verification completed"
}

case "${os_name}" in
  Linux) install_linux_deps ;;
  Darwin) install_macos_deps ;;
  *) warn "Unsupported OS ${os_name}; dependency installation skipped." ;;
esac

download_onnxruntime
download_libtorch
detect_tensorrt
write_env_file
build_and_verify

cat <<EOF

ShortHand build infrastructure is ready.

Next commands:
  source ./shorthand_env.sh
  bash scripts/smoke_test.sh

Third-party SDK location:
  ${THIRD_PARTY_DIR}

Notes:
  - LLVM/Clang are installed through the OS package manager.
  - ONNX Runtime is downloaded under third_party/ by default.
  - LibTorch is optional; run with DOWNLOAD_LIBTORCH=1 to download it.
  - TensorRT is optional and NVIDIA-license/platform-gated; install it separately and set TENSORRT_ROOT.
EOF
