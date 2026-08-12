#!/usr/bin/env bash
set -euo pipefail

ENVIRONMENT_JSON="${1:-}"
POLICIES_JSON="${2:-}"
if [[ -z "${ENVIRONMENT_JSON}" || -z "${POLICIES_JSON}" ]]; then
  echo "usage: $0 <environment-json> <deployment-policies-json>" >&2
  exit 2
fi
for file in "${ENVIRONMENT_JSON}" "${POLICIES_JSON}"; do
  [[ -s "${file}" ]] || { echo "error: required release environment policy evidence missing: ${file}" >&2; exit 1; }
done

compact_env="$(tr -d '\r\n\t ' < "${ENVIRONMENT_JSON}")"
compact_policies="$(tr -d '\r\n\t ' < "${POLICIES_JSON}")"

[[ "${compact_env}" == *'"name":"production-release"'* ]] || {
  echo "error: release job is not bound to the production-release environment" >&2
  exit 1
}
[[ "${compact_env}" == *'"type":"required_reviewers"'* ]] || {
  echo "error: production-release must require reviewers" >&2
  exit 1
}
[[ "${compact_env}" == *'"prevent_self_review":true'* ]] || {
  echo "error: production-release must prevent self-review" >&2
  exit 1
}
if [[ ! "${compact_env}" =~ \"reviewers\":\[\{ ]]; then
  echo "error: production-release required reviewer list is empty" >&2
  exit 1
fi
[[ "${compact_env}" == *'"custom_branch_policies":true'* ]] || {
  echo "error: production-release must use custom deployment branch/tag policies" >&2
  exit 1
}
[[ "${compact_env}" == *'"protected_branches":false'* ]] || {
  echo "error: production-release must use explicit tag policy rather than generic protected-branch admission" >&2
  exit 1
}
if [[ ! "${compact_policies}" =~ \"name\":\"v\*\" ]]; then
  echo "error: production-release deployment policy must contain the exact v* tag pattern" >&2
  exit 1
fi

printf 'PASS protected release environment policy required_reviewers=true prevent_self_review=true tag_policy=v*\n'
