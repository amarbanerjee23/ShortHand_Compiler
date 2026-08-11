# ShortHand toolchain, platform, CTest and reproducibility contract

toolchain_platform_contract_version: shorthand.portability.reproducibility.v1
roadmap_implementation: PR74
github_pull_request_target: PR75
language_version: beta-0.3
production_claim: false

## Purpose

This contract implements the roadmap work originally named PR74. GitHub pull request number 74 was consumed by the earlier duplicate safety branch, so the implementation is carried by GitHub PR75 without renumbering the remaining roadmap responsibilities.

The objective is to replace a single-host portability assumption with executable qualification evidence. A platform or compiler is qualified only when its mandatory GitHub Actions job succeeds on the exact pull-request head. Presence of a runner label, compiler binary or architecture string is not execution evidence.

## Qualification tiers

### Full compiler and runtime qualification

The PR75 matrix targets the following full compiler/runtime environments:

| Environment | Architecture | C++ toolchain | LLVM contract | Required evidence |
| --- | --- | --- | --- | --- |
| Ubuntu 24.04 | x86-64 | GCC 12 | LLVM 18 | CMake configure/build, compiler smoke, runtime build |
| Ubuntu 24.04 | x86-64 | GCC 14 | LLVM 18 | CMake configure/build, compiler smoke, runtime build |
| Ubuntu 24.04 | x86-64 | Clang 16 | LLVM 18 | CMake configure/build, compiler smoke, runtime build |
| Ubuntu 24.04 | x86-64 | Clang 18 | LLVM 18 | CMake configure/build, compiler smoke, runtime build |
| Ubuntu 24.04 | arm64 | distribution Clang/LLVM | runner LLVM | compiler/runtime build, portable CTest smoke, installed consumer |
| macOS 15 | Apple Silicon arm64 | Homebrew Clang/LLVM 18 | LLVM 18 | compiler/runtime build, portable CTest smoke, installed consumer |
| Windows Server 2025 | x86-64 | MSYS2 UCRT64 Clang | MSYS2 LLVM | compiler/runtime build, interpreter/bitcode smoke, installed consumer |

The Windows job uses the GitHub-hosted Windows image but executes the compiler build in a controlled MSYS2 UCRT64 toolchain so the Flex/Bison/LLVM dependencies and POSIX shell build helpers are explicit. A successful Windows runner allocation by itself is not qualification.

### Unsupported or unqualified combinations

The matrix does not claim support for:

- Windows MSVC compiler construction,
- macOS Intel,
- Linux distributions other than the declared Ubuntu LTS runner,
- LLVM versions that are not exercised by the mandatory matrix,
- live GPU, TPU or NPU execution.

Those combinations must not be described as production-qualified based on this PR. Backend/device execution remains governed by the later hardware qualification roadmap item.

## CTest parity contract

ctest_parity_contract_version: shorthand.ctest.make-parity.v1

The canonical Makefile `test` aggregate expands to these targets:

- `test-unit`
- `test-integration`
- `test-green`
- `test-compiler`
- `test-negative`
- `test-diagnostics`
- `test-runtime-linking`
- `test-conformance`
- `test-semantic-differential`
- `test-llvm`
- `test-ai`

`tests/ctest_parity/CMakeLists.txt` registers exactly those Make-backed tests as first-class CTest tests. `scripts/check_cmake_ctest_parity.sh` verifies the mapping mechanically before execution and the CI `ctest-parity` job runs the mapped suite serially. Safety-specific fuzz, memory-sanitizer and ThreadSanitizer gates stay independently mandatory and are not hidden inside the parity layer.

## Installed consumer contract

installed_consumer_contract_version: shorthand.installed.consumer.v1

The repository includes a checked-in external consumer project under `tests/packaging/installed_consumer`. The consumer is configured only against the installed prefix using `find_package(ShortHand 1 CONFIG REQUIRED)`. It builds and executes static/shared runtime consumers and static/shared AI bridge consumers. The platform jobs therefore prove that the install tree is usable by a downstream CMake project instead of merely checking that files were copied.

The Linux baseline retains the existing pkg-config, SONAME and frozen-symbol checks. Cross-platform jobs add CMake package consumption. No successful in-tree link is allowed to substitute for installed-package evidence.

## Reproducible build contract

reproducible_build_contract_version: shorthand.reproducible.build.v1

`scripts/check_reproducible_builds.sh` performs two independent clean Release builds in different build directories with:

- `SOURCE_DATE_EPOCH` fixed,
- `LC_ALL=C` and `TZ=UTC`,
- source/build path remapping through compiler prefix-map flags,
- the same logical install prefix with separate `DESTDIR` roots,
- no CMake cache reuse between build A and build B.

The gate compares SHA-256 manifests for every regular installed file and also compares the CMake-built `short_hand` and `green_ai_tool` executables. A deliberate tamper self-test proves the comparison path rejects a modified manifest. The gate emits `/tmp/shorthand_reproducibility.json` with the observed compiler, LLVM version, architecture and manifest hash.

Reproducibility is exact byte-for-byte equivalence for the declared Ubuntu Release build. The gate must fail rather than normalize away a binary difference.

## CI DAG and stable status contract

The CI workflow is decomposed into independent jobs:

1. `ubuntu-core` preserves all inherited correctness, fuzz, sanitizer, enterprise, Makefile, CMake and CTest gates.
2. `toolchain` qualifies the declared GCC/Clang matrix.
3. `linux-arm64` qualifies native Linux arm64 and its installed consumer.
4. `macos-arm64` qualifies Apple Silicon and its installed consumer.
5. `windows-x64` qualifies the MSYS2 UCRT64 Windows compiler/runtime path and installed consumer.
6. `ctest-parity` executes the mechanically mapped Makefile test aggregate through CTest.
7. `reproducible` performs independent clean-build equivalence.
8. `ubuntu` is the aggregate status publisher and succeeds only when every mandatory dependency succeeds.

The stable required commit-status contexts remain exactly:

- `ci / ubuntu (push)`
- `ci / ubuntu (pull_request)`

Cancelled superseded runs remain unable to publish a terminal failure onto a shared SHA.

## Claim boundary

This PR is portability and build-system evidence, not a production-readiness declaration. It does not prove lower energy use than Python, live accelerator correctness, signed release provenance, deployment hardening or complete MLIR lowering. `production_claim: false` remains mandatory until the later production blockers are closed.