# PR task stability strategy

## Purpose

ShortHand now has many accumulated CI gates. A new PR must not break old tasks while adding new validation. This strategy defines a lightweight preflight contract for future PRs.

## Old-task contract

Do not rename or remove stable phrases that existing CI gates use as tracking anchors. In particular, keep the feature tracker terms used by `scripts/check_feature_plan_status.sh`, including:

- `Automated SBOM`
- `Runtime observability implementation`
- `Module/import/package model`
- `Production blockers`
- `Compiled-code metadata/runtime lowering`

When a PR wants to improve a topic, add detail around the existing anchor phrase instead of replacing the phrase.

## New-gate contract

A new gate should be added only when it is:

1. dependency-free, or safely skipped when optional dependencies are unavailable,
2. checked with `bash -n` before it is wired into CI,
3. anchored by a clear `PASS ...` message,
4. conservative about production, certification, and release-readiness claims,
5. exercised by enterprise hardening only after the old invariants are preserved.

## Practical PR sequence

Before opening or updating a PR, run the light checks first:

```bash
bash scripts/check_feature_plan_status.sh
bash scripts/check_pr_task_stability.sh
bash scripts/check_enterprise_hardening.sh
```

Only after these pass should a PR rely on heavier compiler/build tasks such as Makefile tests, sanitizer, CMake build, and CTest.

## Why this exists

Recent PR failures were not caused by core compiler regressions. They were mostly caused by brittle validation wording, path assumptions, and new gates being wired before their claim-safety behavior was proven. This file makes that process explicit so future PRs preserve old task contracts before expanding validation.
