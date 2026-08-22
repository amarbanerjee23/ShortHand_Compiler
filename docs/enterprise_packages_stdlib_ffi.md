# Enterprise packages, core library and FFI contract

enterprise_contract: shorthand.enterprise_language.v1
package_manifest: shorthand.package.v2
package_lock: shorthand.lock.v2
package_graph: shorthand.module.graph.v2
core_ffi_abi: 1.0.0
language_version: beta-0.6
release_status: controlled_beta
production_claim: false
c3eco_alignment: evidence_integrity_repeatability_security_and_no_quality_degradation

## Purpose and claim boundary

This contract adds a bounded enterprise language and supply-chain surface without claiming that ShortHand as a whole is production ready or C3-ECO certified. It preserves the beta-0.5 executable compiler semantics and the frozen runtime ABI 1.0.0 with its existing 25 `short_*` symbols. The new core library has a separate C ABI and symbol allowlist.

The `enterprise-check` compiler mode validates ABI schemas and ownership plans. It does not lower composite values into interpreter or LLVM execution. Records, enums, slices, options and results therefore become source-level, versioned ABI-design inputs, while by-value composite calls and owned string arrays remain unavailable. This distinction is intentional and fail closed.

## Enterprise source surface

An enterprise source begins with these ordered declarations:

```text
language shorthand.enterprise_language.v1;
namespace policyclub.sustainable_ai;
```

It may then declare:

- named records with unique scalar `bool`, `int32`, `float64` or `string` fields;
- named enums with unique variants;
- named `slice`, `option` and `result` descriptors with scalar payloads;
- initialized owned values;
- explicit `move`, shared or mutable `borrow`, `release`, `read` and `assign` operations.

The namespace uses the same dotted identifier discipline as package modules. Type identities are deterministic and namespace-qualified in validation evidence. Ownership uses the `shorthand.type_memory.v1` state machine. Use after move, conflicting borrows, assignment during a borrow, owner access during a mutable borrow, unknown releases and unreleased borrows fail with `SHD3016`. Malformed or unsupported enterprise syntax fails with `SHD3024`; duplicate type or value identities fail with `SHD3025`.

`enterprise-check` emits deterministic `shorthand.enterprise_language.v1` JSON with canonical type identities and ownership operation counts. It inherits the compiler's 4 MiB source, 16 KiB token/line and lower-only environment ceilings, and the output always includes `production_claim: false`.

## Offline package manifest v2

`shorthand.package.v2` retains explicit module mappings and adds:

```text
format shorthand.package.v2
package acme.application
version 1.4.0
license Apache-2.0
module acme.application.main src/main.short
dependency acme.math 2.1.0 vendor/acme.math sha256:<64-lowercase-hex> MIT
```

Package and dependency versions are exact semantic versions. Ranges, wildcards and unbounded labels are rejected with `SHD2033`. Redistributed licenses must use the repository allowlist and otherwise fail with `SHD2032`.

Dependencies are local vendored directories beneath the root package. The resolver performs no registry or network access. It verifies the child manifest SHA-256, exact package identity, version and license before adding the child's modules to the graph. Manifest inputs are bounded to 1 MiB, 16 KiB per line and 4,096 lines before graph construction. The child must itself be `shorthand.package.v2`; nested dependencies are rejected in v1 of this contract instead of being silently ignored. Missing, changed, oversized or malformed dependency evidence fails with `SHD2031`.

Every module retains its owning package namespace. Source `package` declarations are checked against that owner, including vendored modules. Duplicate module identities and path aliases remain rejected. Function/global collisions still fail before lowering; qualified member dispatch is not introduced by this contract.

The deterministic `shorthand.lock.v2` contains root identity, exact dependency rows and SHA-256 for every reachable source. A changed dependency manifest fails during resolution, and a changed module fails lock verification. `module-graph` emits `shorthand.module.graph.v2` with `offline_only: true`. `package-sbom` requires a verified v2 lock and a valid non-negative `SOURCE_DATE_EPOCH`; missing reproducibility metadata fails with `SHD2034`. It emits SPDX 2.3 creation information, package and `DEPENDS_ON` records, and a typed external reference for each dependency-manifest SHA-256. Package entries use `filesAnalyzed: false` because the command does not claim SPDX file analysis or a whole-directory checksum.

`shorthand.package.v1` and `shorthand.lock.v1` remain accepted unchanged for compatibility. Their historical FNV-1a graph fingerprint is not upgraded in place and is not a cryptographic dependency claim.

## Core standard library and safe FFI

The installable `ShortHand::core` and `ShortHand::core_shared` targets expose `abi/shorthand_core_ffi_v1.h`. The ABI version is 1.0.0 and its exact 15-symbol set is frozen in `abi/core_ffi_public_symbols_v1.txt`.

The bounded v1 library provides:

- validated UTF-8 owned strings with explicit creation, clone, borrowed view and destruction;
- read-only bounded `int32` slices;
- `float64` option and `int32` result carriers;
- fixed-width 32-bit status values and stable names;
- a move-aware `shorthand::core::v1::String` C++ RAII wrapper.

All C entry points prevent C++ exceptions from crossing the ABI. Allocation failure becomes a status. Strings allocated by the library are released only by `short_core_string_destroy`. Views do not transfer ownership and expire when their owner is destroyed or mutated. Null arguments, invalid UTF-8, out-of-range indices and wrong variants fail deterministically. Composite records and enums are not passed by value across the ABI; callers use the stable opaque/string and fixed carrier types only.

Installed C and C++ consumers build and run against static and shared core targets on every existing platform lane. Linux additionally compares dynamic exports to the frozen symbol allowlist. Sanitizer execution covers UTF-8, ownership, cloning, view, option/result and bounds paths.

## C3-ECO alignment

Cryptographic dependency identity, exact versions, allowlisted licenses, an SPDX dependency graph, deterministic type identity, lifetime enforcement and bounded FFI operations support the repository's G6 security, G7 safety/privacy/accessibility, G8 repeatability and G13 no-quality-degradation evidence controls. They protect the meaning and reproducibility of later measurement and certification evidence. They do not demonstrate energy or carbon savings, calculate certification scores, authorize a public claim or grant certification.

## Evidence

`scripts/check_enterprise_packages_stdlib_ffi.sh` is the first-class acceptance gate. It covers positive and negative language validation, SHA-256 known-answer vectors, dependency and source tamper detection, offline dependency failure, exact-version and license policy, deterministic lock/SBOM output, C/C++ ABI behavior, exported symbols, installed-consumer declarations and sanitizer execution. The same gate is registered in direct CI, Make governance, the Make aggregate and CTest.
