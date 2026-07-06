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
