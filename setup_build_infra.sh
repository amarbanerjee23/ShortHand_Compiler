#!/usr/bin/env bash
set -euo pipefail

# OS-aware setup for the ShortHand compiler and C++ GreenAI toolchain.
# It installs or detects LLVM/Clang/Flex/Bison, downloads the correct ONNX
# Runtime C++ SDK for the current OS/CPU, optionally downloads LibTorch, writes
# shorthand_env.sh, builds the language, and runs verification.
#
# Usage:
#   bash setup_build_infra.sh
#   source ./shorthand_env.sh
#   bash scripts/smoke_test.sh
#
# Options:
#   LLVM_VERSION=18 bash setup_build_infra.sh
#   DOWNLOAD_ONNXRUNTIME=0 bash setup_build_infra.sh
#   ONNXRUNTIME_VERSION=1.20.1 bash setup_build_infra.sh
#   DOWNLOAD_LIBTORCH=1 bash setup_build_infra.sh
#   LIBTORCH_URL=<official-url> bash setup_build_infra.sh
#   TENSORRT_ROOT=/path/to/TensorRT bash setup_build_infra.sh
#   RUN_SMOKE_TEST=0 bash setup_build_infra.sh
#   SKIP_SYSTEM_PACKAGES=1 bash setup_build_infra.sh

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
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
SKIP_SYSTEM_PACKAGES="${SKIP_SYSTEM_PACKAGES:-0}"

mkdir -p "${THIRD_PARTY_DIR}" "${DOWNLOAD_DIR}" "${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"

log(){ printf '\n[setup] %s\n' "$*"; }
warn(){ printf '\n[setup][warning] %s\n' "$*" >&2; }
fail(){ printf '\n[setup][error] %s\n' "$*" >&2; exit 1; }
have(){ command -v "$1" >/dev/null 2>&1; }

KERNEL="$(uname -s)"
ARCH_RAW="$(uname -m)"
case "${ARCH_RAW}" in
  x86_64|amd64) ARCH="x64" ;;
  aarch64|arm64) ARCH="arm64" ;;
  *) ARCH="${ARCH_RAW}" ;;
esac

OS_FAMILY="unknown"
case "${KERNEL}" in
  Linux) OS_FAMILY="linux" ;;
  Darwin) OS_FAMILY="macos" ;;
  *) OS_FAMILY="unknown" ;;
esac

OS_ID="unknown"
OS_VERSION_ID="unknown"
if [[ -r /etc/os-release ]]; then
  . /etc/os-release
  OS_ID="${ID:-unknown}"
  OS_VERSION_ID="${VERSION_ID:-unknown}"
fi

SUDO=""
if [[ "${EUID:-$(id -u)}" -ne 0 ]] && have sudo; then SUDO="sudo"; fi

log "Detected OS=${OS_FAMILY} distro=${OS_ID} version=${OS_VERSION_ID} arch=${ARCH}"

install_packages(){
  [[ "${SKIP_SYSTEM_PACKAGES}" == "1" ]] && { warn "Skipping system package installation"; return; }
  if [[ "${OS_FAMILY}" == "macos" ]]; then
    have brew || { warn "Homebrew not found. Install Homebrew or set SKIP_SYSTEM_PACKAGES=1 and LLVM_CONFIG manually."; return; }
    log "Installing macOS dependencies with Homebrew"
    brew install llvm flex bison cmake ninja curl git || true
  elif have apt-get; then
    log "Installing Debian/Ubuntu dependencies"
    ${SUDO} apt-get update
    local llvm_pkgs="llvm llvm-dev clang"
    [[ -n "${LLVM_VERSION}" ]] && llvm_pkgs="llvm-${LLVM_VERSION} llvm-${LLVM_VERSION}-dev clang-${LLVM_VERSION}"
    ${SUDO} apt-get install -y build-essential make cmake ninja-build curl ca-certificates unzip tar pkg-config git flex bison libfl-dev ${llvm_pkgs}
  elif have dnf; then
    log "Installing Fedora/RHEL dependencies with dnf"
    ${SUDO} dnf install -y gcc gcc-c++ make cmake ninja-build curl unzip tar pkgconf-pkg-config git flex bison llvm llvm-devel clang
  elif have yum; then
    log "Installing RHEL/CentOS dependencies with yum"
    ${SUDO} yum install -y gcc gcc-c++ make cmake ninja-build curl unzip tar pkgconfig git flex bison llvm llvm-devel clang
  elif have pacman; then
    log "Installing Arch dependencies"
    ${SUDO} pacman -Sy --needed --noconfirm base-devel cmake ninja curl unzip tar pkgconf git flex bison llvm clang
  elif have zypper; then
    log "Installing openSUSE dependencies"
    ${SUDO} zypper --non-interactive install -y gcc gcc-c++ make cmake ninja curl unzip tar pkg-config git flex bison llvm llvm-devel clang
  else
    warn "No supported package manager found. Install LLVM/Clang/Flex/Bison/libfl manually."
  fi
}

find_exec(){
  local x
  for x in "$@"; do
    [[ -x "${x}" ]] && { echo "${x}"; return 0; }
    have "${x}" && { command -v "${x}"; return 0; }
  done
  return 1
}

resolve_llvm_config(){
  [[ -n "${LLVM_CONFIG:-}" && -x "${LLVM_CONFIG}" ]] && { echo "${LLVM_CONFIG}"; return; }
  local c=()
  [[ -n "${LLVM_VERSION}" ]] && c+=("llvm-config-${LLVM_VERSION}" "/usr/lib/llvm-${LLVM_VERSION}/bin/llvm-config" "/opt/homebrew/opt/llvm@${LLVM_VERSION}/bin/llvm-config" "/usr/local/opt/llvm@${LLVM_VERSION}/bin/llvm-config")
  c+=("llvm-config" "/usr/lib/llvm/bin/llvm-config" "/usr/lib/llvm-20/bin/llvm-config" "/usr/lib/llvm-19/bin/llvm-config" "/usr/lib/llvm-18/bin/llvm-config" "/usr/lib/llvm-17/bin/llvm-config" "/usr/lib/llvm-16/bin/llvm-config" "/usr/lib/llvm-15/bin/llvm-config" "/usr/lib/llvm-14/bin/llvm-config" "/opt/homebrew/opt/llvm/bin/llvm-config" "/usr/local/opt/llvm/bin/llvm-config")
  find_exec "${c[@]}" || fail "llvm-config not found. Set LLVM_CONFIG=/path/to/llvm-config or install LLVM."
}

resolve_tool_near_llvm(){
  local tool="$1"
  local llvm_bin="$2"
  local c=()
  [[ -n "${LLVM_VERSION}" ]] && c+=("${tool}-${LLVM_VERSION}")
  c+=("${llvm_bin}/${tool}" "${tool}" "/opt/homebrew/opt/llvm/bin/${tool}" "/usr/local/opt/llvm/bin/${tool}")
  find_exec "${c[@]}" || echo "${tool}"
}

latest_ort_version(){
  curl -fsSL "https://api.github.com/repos/microsoft/onnxruntime/releases/latest" | sed -n 's/.*"tag_name": *"v\([^"]*\)".*/\1/p' | head -n 1
}

ort_asset_prefix(){
  case "${OS_FAMILY}-${ARCH}" in
    linux-x64) echo "onnxruntime-linux-x64" ;;
    linux-arm64) echo "onnxruntime-linux-aarch64" ;;
    macos-x64) echo "onnxruntime-osx-x86_64" ;;
    macos-arm64) echo "onnxruntime-osx-arm64" ;;
    *) return 1 ;;
  esac
}

download_onnxruntime(){
  [[ "${DOWNLOAD_ONNXRUNTIME}" == "1" ]] || { warn "ONNX Runtime download disabled"; return; }
  have curl || fail "curl is required"
  have tar || fail "tar is required"
  local version="${ONNXRUNTIME_VERSION}"
  [[ "${version}" == "latest" ]] && version="$(latest_ort_version)"
  [[ -n "${version}" ]] || fail "Could not resolve ONNX Runtime version"
  local prefix; prefix="$(ort_asset_prefix)" || { warn "No ONNX Runtime asset for ${OS_FAMILY}-${ARCH}"; return; }
  local archive="${prefix}-${version}.tgz"
  local url="https://github.com/microsoft/onnxruntime/releases/download/v${version}/${archive}"
  local dest="${DOWNLOAD_DIR}/${archive}"
  local out="${THIRD_PARTY_DIR}/onnxruntime-${version}-${prefix}"
  if [[ ! -d "${out}" ]]; then
    log "Downloading ONNX Runtime ${version} for ${OS_FAMILY}/${ARCH}"
    curl -fL --retry 3 "${url}" -o "${dest}"
    mkdir -p "${out}"
    tar -xzf "${dest}" -C "${out}" --strip-components=1
  else
    log "ONNX Runtime already exists at ${out}"
  fi
  ln -sfn "${out}" "${THIRD_PARTY_DIR}/onnxruntime"
}

default_libtorch_url(){
  case "${OS_FAMILY}-${ARCH}" in
    linux-x64) echo "https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcpu.zip" ;;
    macos-arm64) echo "https://download.pytorch.org/libtorch/cpu/libtorch-macos-arm64-2.5.1.zip" ;;
    macos-x64) echo "https://download.pytorch.org/libtorch/cpu/libtorch-macos-x86_64-2.5.1.zip" ;;
    *) return 1 ;;
  esac
}

download_libtorch(){
  [[ "${DOWNLOAD_LIBTORCH}" == "1" ]] || { warn "LibTorch download disabled"; return; }
  have curl || fail "curl is required"
  have unzip || fail "unzip is required"
  local url="${LIBTORCH_URL}"
  [[ -z "${url}" ]] && url="$(default_libtorch_url)" || true
  [[ -n "${url}" ]] || fail "No LibTorch URL for ${OS_FAMILY}-${ARCH}; set LIBTORCH_URL"
  local archive="${DOWNLOAD_DIR}/libtorch-${OS_FAMILY}-${ARCH}.zip"
  local out="${THIRD_PARTY_DIR}/libtorch"
  if [[ ! -d "${out}" ]]; then
    log "Downloading LibTorch for ${OS_FAMILY}/${ARCH}"
    curl -fL --retry 3 "${url}" -o "${archive}"
    rm -rf "${THIRD_PARTY_DIR}/libtorch_tmp"
    mkdir -p "${THIRD_PARTY_DIR}/libtorch_tmp"
    unzip -q "${archive}" -d "${THIRD_PARTY_DIR}/libtorch_tmp"
    [[ -d "${THIRD_PARTY_DIR}/libtorch_tmp/libtorch" ]] || fail "LibTorch archive layout not recognized"
    mv "${THIRD_PARTY_DIR}/libtorch_tmp/libtorch" "${out}"
    rm -rf "${THIRD_PARTY_DIR}/libtorch_tmp"
  else
    log "LibTorch already exists at ${out}"
  fi
}

detect_tensorrt(){
  [[ -n "${TENSORRT_ROOT}" && -d "${TENSORRT_ROOT}" ]] && { log "Using TensorRT at ${TENSORRT_ROOT}"; return; }
  for d in /usr/local/TensorRT /usr/src/tensorrt /opt/TensorRT /usr/lib/x86_64-linux-gnu; do
    [[ -e "${d}" ]] && { TENSORRT_ROOT="${d}"; log "Detected possible TensorRT at ${TENSORRT_ROOT}"; return; }
  done
  warn "TensorRT is not auto-downloaded because it is NVIDIA-license/platform-gated. Set TENSORRT_ROOT after installing it."
}

write_env(){
  local llvm_config llvm_bin clang_bin llc_bin onnx_root libtorch_root
  llvm_config="$(resolve_llvm_config)"
  llvm_bin="$(dirname "${llvm_config}")"
  clang_bin="$(resolve_tool_near_llvm clang "${llvm_bin}")"
  llc_bin="$(resolve_tool_near_llvm llc "${llvm_bin}")"
  onnx_root=""; [[ -d "${THIRD_PARTY_DIR}/onnxruntime" ]] && onnx_root="${THIRD_PARTY_DIR}/onnxruntime"
  libtorch_root=""; [[ -d "${THIRD_PARTY_DIR}/libtorch" ]] && libtorch_root="${THIRD_PARTY_DIR}/libtorch"
  cat > "${ENV_FILE}" <<EOF_ENV
# Generated by setup_build_infra.sh
export SHORT_HAND_ROOT="${ROOT_DIR}"
export SHORT_HAND_THIRD_PARTY="${THIRD_PARTY_DIR}"
export SHORT_HAND_OS_FAMILY="${OS_FAMILY}"
export SHORT_HAND_ARCH="${ARCH}"
export LLVM_CONFIG="${llvm_config}"
export CLANG_BIN="${clang_bin}"
export LLC_BIN="${llc_bin}"
export ONNXRUNTIME_ROOT="${onnx_root}"
export LIBTORCH_ROOT="${libtorch_root}"
export TENSORRT_ROOT="${TENSORRT_ROOT}"
export PATH="${llvm_bin}:$(dirname "${clang_bin}"):$(dirname "${llc_bin}"):\${PATH}"
EOF_ENV
  log "Wrote ${ENV_FILE}"
}

build_verify(){
  # shellcheck source=/dev/null
  source "${ENV_FILE}"
  log "LLVM version: $("${LLVM_CONFIG}" --version)"
  log "Building C++ GreenAI tool"
  make -C "${SRC_DIR}" green_ai_tool
  log "Running C++ GreenAI tests"
  make -C "${SRC_DIR}" test-green
  log "Building ShortHand compiler"
  make -C "${SRC_DIR}" clean all LLVM_CONFIG="${LLVM_CONFIG}"
  log "Building AI inference binary"
  if [[ -n "${ONNXRUNTIME_ROOT}" ]]; then make -C "${SRC_DIR}" ai_app ONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}"; else make -C "${SRC_DIR}" ai_app; fi
  log "Building AI training binary"
  if [[ -n "${LIBTORCH_ROOT}" ]]; then make -C "${SRC_DIR}" ai_train LIBTORCH_ROOT="${LIBTORCH_ROOT}"; else make -C "${SRC_DIR}" ai_train; fi
  if [[ "${RUN_SMOKE_TEST}" == "1" ]]; then bash "${ROOT_DIR}/scripts/smoke_test.sh"; else warn "RUN_SMOKE_TEST=0; smoke test skipped"; fi
}

install_packages
download_onnxruntime
download_libtorch
detect_tensorrt
write_env
build_verify

cat <<EOF_DONE

ShortHand build infrastructure is ready.
Detected: OS=${OS_FAMILY}, distro=${OS_ID}, version=${OS_VERSION_ID}, arch=${ARCH}
Third-party SDKs are under: ${THIRD_PARTY_DIR}

Next commands:
  source ./shorthand_env.sh
  bash scripts/smoke_test.sh

Notes:
  - LLVM/Clang are installed/detected based on the OS/package manager.
  - ONNX Runtime is selected by OS and architecture.
  - LibTorch is optional: DOWNLOAD_LIBTORCH=1 bash setup_build_infra.sh
  - TensorRT is optional and must be installed separately; set TENSORRT_ROOT.
EOF_DONE
