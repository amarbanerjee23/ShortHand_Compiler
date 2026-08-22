# Runtime production packaging

runtime_packaging_contract_version: 1.0.0
runtime_packaging_status: installable_static_shared_and_consumer_checked
runtime_shared_version: 1.0.0
runtime_shared_soversion: 1
ai_bridge_packaging_status: adapter_static_shared_and_consumer_checked
core_packaging_status: core_ffi_static_shared_and_consumer_checked
cmake_package_name: ShortHand
pkg_config_runtime_name: shorthand-runtime
pkg_config_ai_bridge_name: shorthand-ai-bridge
pkg_config_core_name: shorthand-core
production_claim_boundary: packaging_gate_is_not_full_production_readiness

## Scope

PR61 turns the frozen runtime ABI and the AI bridge adapter into repeatable installable build artifacts.

The production CMake build installs:

1. `libshorthand_runtime` as static and shared libraries.
2. `libshorthand_ai_bridge` as static and shared libraries.
3. `libshorthand_core` as static and shared libraries with separate FFI ABI 1.0.0.
4. runtime, bridge, frozen runtime ABI, core C ABI and core C++ wrapper headers.
5. `ShortHandConfig.cmake`, `ShortHandConfigVersion.cmake`, and exported `ShortHandTargets.cmake` metadata.
6. runtime, AI bridge and core pkg-config metadata.

## Installed CMake targets

The installed package exports these targets:

- `ShortHand::runtime` for the static runtime.
- `ShortHand::runtime_shared` for the shared runtime.
- `ShortHand::ai_bridge` for the static C++ adapter.
- `ShortHand::ai_bridge_shared` for the shared C++ adapter.
- `ShortHand::core` and `ShortHand::core_shared` for the safe core/FFI library.

A downstream CMake project uses `find_package(ShortHand 1 CONFIG REQUIRED)` and links exactly one static or shared variant for each component it needs.

## Shared-library identity

All three shared libraries use version `1.0.0` and ABI SOVERSION `1`.

On ELF systems this produces a SONAME compatible with:

- `libshorthand_runtime.so.1`
- `libshorthand_ai_bridge.so.1`
- `libshorthand_core.so.1`

The full artifact remains versioned as `1.0.0`. Platform-specific CMake naming is used on macOS and Windows.

## Consumer evidence

`tests/packaging/test_runtime_production_packaging.sh` performs a clean out-of-tree configure, build, install and downstream consumer build. It verifies:

1. static and shared runtime artifacts,
2. static and shared AI bridge artifacts,
3. installed public and frozen headers,
4. CMake package discovery and target linkage,
5. pkg-config discovery and linkage when pkg-config is available,
6. runtime ABI version and basic registry behavior,
7. bridge adapter contract version and status mapping,
8. core C and C++ static/shared consumers plus exact core exported symbols,
9. ELF SONAME values when `readelf` is available.

The gate uses only the installation prefix for downstream compilation. Repository include paths and build-tree library paths are not allowed in the consumer project.

## Boundaries

The packaged `shorthand_ai_bridge` library contains the dependency-light C++ mapping adapter and AI type conversion helpers. It does not package third-party backend SDKs and it does not claim that ONNX Runtime, TensorRT, OpenVINO, LibTorch or llama.cpp executed successfully.

The runtime remains one process-wide serialized ABI v1 context. Packaging does not add tenant handles, parallel backend execution, network exporters, deployment hardening, signed releases or a full production-readiness claim.

PR62 remains responsible for the Prometheus scrape endpoint host adapter.
