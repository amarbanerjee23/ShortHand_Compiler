#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="${1:-}"
MODE="${2:-}"

if [[ -z "${VERSION}" ]]; then
  echo "usage: $0 <vMAJOR.MINOR.PATCH[-rc.N]> [--publish]" >&2
  exit 2
fi

if [[ ! "${VERSION}" =~ ^v([0-9]+)\.([0-9]+)\.([0-9]+)(-rc\.([0-9]+))?$ ]]; then
  echo "error: release version must match vMAJOR.MINOR.PATCH or vMAJOR.MINOR.PATCH-rc.N" >&2
  exit 1
fi

cmake_version="$(sed -nE 's/^project\(ShortHand VERSION ([0-9]+\.[0-9]+\.[0-9]+) LANGUAGES CXX\)$/\1/p' "${ROOT_DIR}/CMakeLists.txt")"
if [[ -z "${cmake_version}" ]]; then
  echo "error: unable to resolve ShortHand project version from CMakeLists.txt" >&2
  exit 1
fi

base_version="${VERSION#v}"
base_version="${base_version%%-rc.*}"
if [[ "${base_version}" != "${cmake_version}" ]]; then
  echo "error: tag base version ${base_version} does not match CMake project version ${cmake_version}" >&2
  exit 1
fi

if [[ "${MODE}" == "--publish" ]]; then
  if [[ "${GITHUB_REF_TYPE:-}" != "tag" ]]; then
    echo "error: publication is allowed only from a GitHub tag event" >&2
    exit 1
  fi
  if [[ "${GITHUB_REF_NAME:-}" != "${VERSION}" ]]; then
    echo "error: requested version does not match GITHUB_REF_NAME" >&2
    exit 1
  fi

  head_sha="$(git -C "${ROOT_DIR}" rev-parse HEAD)"
  expected_sha="${GITHUB_SHA:-${head_sha}}"
  if [[ "${head_sha}" != "${expected_sha}" ]]; then
    echo "error: checkout HEAD does not match the GitHub event SHA" >&2
    exit 1
  fi

  tag_sha="$(git -C "${ROOT_DIR}" rev-parse "${VERSION}^{commit}" 2>/dev/null || true)"
  if [[ -z "${tag_sha}" || "${tag_sha}" != "${head_sha}" ]]; then
    echo "error: immutable release tag does not resolve to the exact checkout commit" >&2
    exit 1
  fi

  release_base_ref="${SHORTHAND_RELEASE_BASE_REF:-refs/remotes/origin/master}"
  if ! git -C "${ROOT_DIR}" rev-parse --verify --quiet "${release_base_ref}^{commit}" >/dev/null; then
    echo "error: protected release base ref is unavailable: ${release_base_ref}" >&2
    exit 1
  fi
  if ! git -C "${ROOT_DIR}" merge-base --is-ancestor "${tag_sha}" "${release_base_ref}"; then
    echo "error: release tag commit is not reachable from protected master lineage: ${release_base_ref}" >&2
    exit 1
  fi
elif [[ -n "${MODE}" ]]; then
  echo "error: unsupported release policy mode: ${MODE}" >&2
  exit 2
fi

printf 'PASS release version policy version=%s project_version=%s mode=%s\n' "${VERSION}" "${cmake_version}" "${MODE:---dry-run}"
