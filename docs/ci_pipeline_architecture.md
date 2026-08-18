# ShortHand CI and release pipeline architecture

ci_pipeline_architecture_version: 2026-08-09-v1
pipeline_maturity: controlled_beta
production_claim: false

## Purpose

The pipeline must prove that ShortHand remains a deterministic, memory-safe, portable compiled AI language while feature work continues. Fast feedback, failure isolation and honest hardware/security/deployment evidence are required. A skipped dependency, cancelled run, unavailable accelerator, optional SDK, unavailable security scanner, unavailable container runtime or missing cluster must never be reported as successful production execution.

## Non-negotiable merge contract

Every implementation PR must finish on its final head with both stable event-specific statuses successful:

- `ci / ubuntu (push)`
- `ci / ubuntu (pull_request)`

Cancelled superseded runs do not publish a terminal custom status. Mandatory tests may not use `continue-on-error`, unconditional skips or retry loops that convert deterministic failures into success.

## Pipeline layers

### Tier 0 - CI policy and repository invariants

Runs first and fails quickly.

- workflow/status hygiene,
- roadmap and feature-plan consistency,
- required-file and claim-safety guards,
- immutable GitHub Action and toolchain version policy,
- third-party inventory/license/exception policy,
- container/Kubernetes static and negative policy contract,
- generated-file freshness checks.

### Tier 1 - frontend correctness

- Bison/Flex regeneration with conflicts as errors,
- grammar/conformance matrices,
- AST/source ranges,
- diagnostics codes,
- module manifest/graph/lockfile determinism,
- malformed and negative corpus.

### Tier 2 - compiler functional correctness

- Makefile suite,
- CMake configure/build,
- CTest parity,
- interpreter/LLVM/native differential execution,
- module and separate-compilation linking.

### Tier 3 - memory, undefined behavior and concurrency safety

- ASan,
- LSan,
- UBSan,
- TSan where supported,
- parser/module/compiler fuzz smoke on PRs,
- extended fuzzing on scheduled runs.

### Tier 4 - toolchain and platform qualification

- GCC and Clang,
- pinned supported LLVM versions,
- Linux x86-64 and arm64,
- macOS Apple Silicon,
- Windows,
- installed consumer and ABI checks,
- native production container execution on Linux amd64 and arm64.

### Tier 5 - runtime/backend/hardware qualification

CPU fallback remains mandatory. Accelerator evidence is explicit and capability-aware.

The runtime inventory must distinguish CPU, GPU, TPU and NPU. Selection must consider detected hardware, installed SDK/runtime, model compatibility, precision support, memory and policy. Detection alone is not execution readiness.

Backend jobs must report one of: executed-and-verified, unavailable-with-explicit-reason, or failed. A production-supported backend cannot be released with an unavailable result in the release-candidate matrix.

### Tier 6 - scale and scheduled stress

- 1k+ module graph stress,
- deep dependency graphs,
- parallel compile stress,
- incremental rebuild correctness,
- long-running runtime concurrency,
- extended fuzz corpus,
- daily external vulnerability rescan,
- resource ceilings and timeout enforcement.

These expensive tests run nightly/scheduled while PRs retain bounded smoke versions.

### Tier 7 - release integrity, external security and deployment

- clean reproducible builds,
- SBOM,
- provenance,
- checksums and signatures,
- package/install tests,
- CodeQL C/C++ `security-extended`,
- Trivy vulnerability/secret/misconfiguration/license scan,
- PR dependency review and redistribution license policy,
- expiring security exceptions,
- multi-stage non-root container runtime checks,
- Restricted Pod Security workload validation,
- ephemeral Kubernetes deployment, health, quota, default-deny network and restart evidence.

### Tier 8 - performance and Green AI evidence

- fixed equivalent-workload benchmark corpus,
- compiler latency and peak RSS,
- runtime latency/throughput/memory,
- energy measurements with sensor provenance and uncertainty,
- ShortHand versus equivalent Python baselines,
- regression thresholds that fail closed when measurement quality is inadequate.

## Failure isolation

The mandatory CI DAG has independently named compiler/platform, CTest/reproducibility and external-security jobs. A final aggregate gate depends on every mandatory job and publishes the stable event-specific status. GitHub PR77 added `security` to that dependency set, so CodeQL/Trivy/dependency-policy failures block the same stable contexts as compiler failures.

GitHub PR78 deliberately reuses already-mandatory exact-head lanes rather than creating an advisory deployment workflow. The `ubuntu-core` feature-plan step executes the amd64 production image and pinned ephemeral Kind cluster. The existing native `linux-arm64` installed-SDK lane additionally builds and executes the arm64 production image. Failure of Docker, image construction, health, cluster creation, policy enforcement, quota/network negatives or replica recovery therefore fails the same stable merge contexts.

Each job uploads structured logs even on failure. Artifacts identify the run/commit through GitHub metadata and should include compiler/LLVM versions, test seed, backend inventory and relevant security/release/deployment reports.

## Security execution model

The normal `security` job is read-only. CodeQL produces SARIF with upload disabled and the repository policy parser makes the fail/pass decision. Trivy similarly emits JSON with scanner exit code reserved for execution errors, followed immediately by a mandatory repository-owned report parser. This split preserves machine-readable evidence without turning findings into warnings.

Repository-owned dependency delta review is PR/base-head evidence. Trivy runs on both push and PR and also on the daily `security-rescan` workflow so newly published CVEs can be detected without a source change. The generated vulnerable dependency fixture is under `/tmp` so it cannot enter a release or dependency graph accidentally.

## Deployment execution model

ShortHand is a compiler/CLI workload, not a network service. Production probes execute the real parser against a bundled valid ShortHand source. The production manifest exposes no Service or Ingress and applies default-deny ingress/egress policy. A future network-facing runtime must explicitly add the required protocol, port, authentication and NetworkPolicy contracts rather than weakening this baseline.

The live deployment gate uses version-pinned Kind/Kubernetes inputs and checksum verification. It proves runtime uid/capability/seccomp/no-new-privileges state, ResourceQuota rejection, a positive-control versus denied egress path, two Ready replicas and bounded replacement after pod deletion. Static YAML presence alone does not close TST019.

## Determinism and caching

Caching may accelerate immutable dependencies and generated build intermediates, but correctness must never depend on cache state. Cache keys include OS, architecture, compiler, LLVM version, dependency lock/fingerprint and source inputs. Release qualification always includes at least one clean no-cache build.

Generated parser, lockfile and module-graph outputs must have freshness/determinism guards. Tests use deterministic seeds unless a fuzzing job explicitly records its random seed for replay.

## Timeouts and retries

Every potentially unbounded parser, graph, fuzz, network, scanner, container or runtime test has a declared timeout. Infrastructure/network acquisition may use bounded retry with backoff. Compiler/test/security/deployment finding failures are not retried automatically as a way to obtain green status.

## PR, nightly and release-candidate profiles

PR profile: deterministic mandatory correctness, bounded sanitizer/fuzz smoke, CPU runtime tests, external security scanning and exact-head container/Kubernetes deployment qualification.

Nightly profile: full fuzzing, vulnerability rescan, scale, concurrency, platform and available accelerator matrix.

Release-candidate profile: all declared production platforms/backends/hardware tiers, reproducibility, signing, security, deployment, C3-ECO, performance and energy gates with zero mandatory skips.

## Ownership by roadmap PR

- PR72: semantic differential gate.
- PR73: sanitizer/fuzz/TSan isolation.
- PR74: multi-job DAG, GCC/Clang/platform matrix and reproducibility.
- PR75: signed release publication.
- PR76: security/SAST/dependency/license policy.
- PR77: hardened multi-architecture containers and ephemeral Kubernetes enforcement.
- PR80: CPU/GPU/TPU/NPU and backend execution qualification.
- PR86: performance, energy and zero-skip production RC aggregation.

The remaining tooling, C3-ECO and MLIR PRs add their own mandatory jobs as they become executable contracts.
