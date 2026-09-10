# Evidence Bundle Plan

A release-level branch should keep enough evidence to reproduce and review the build.

## Bundle contents

- commit SHA
- CI run ID
- build command list
- toolchain versions
- validation output
- smoke test output
- Makefile test output
- sanitizer output
- CMake and CTest output
- GreenAI report sample
- known limitations
- skipped optional checks

## AI evidence fields

AI evidence should separate:

- requested backend
- selected backend
- fallback reason
- model path
- tensor shape
- execution status
- measurement tool status

## Current status

The repository already uploads test artifacts in CI. This document defines the expected evidence bundle for later release automation.

## PR91 signed auditor bundle

The native `shorthand_c3eco_audit` contract is documented in [c3eco_auditor_bundle.md](c3eco_auditor_bundle.md). It signs complete artifact inventories, resolves evidence references, replays PR90 assessment, checks lifecycle policy and creates redacted signed public reports. The existing generate_certification_bundle.sh remains a compatible unsigned candidate smoke export. It is not accepted as a signed auditor bundle.
