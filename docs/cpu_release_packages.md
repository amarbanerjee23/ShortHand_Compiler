# CPU package qualification

These are controlled-beta candidate packages, not published or GA-qualified releases. CPU inference on Windows x64 and macOS ARM64 is required alongside Linux. A green compiler portability check alone does not qualify an AI package. The existing default production scope remains Linux x64. On the other three targets, qualification first verifies default refusal, then explicitly sets `SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE=1` for native experiments. Receipts identify `experimental_native_candidate`; they do not promote production support.

## Package contents and prerequisites

`cmake --install` installs `short_hand`, `green_ai_tool`, the existing SDK tools and libraries, CMake targets and pkg-config metadata. With ONNX enabled, the package also includes the pinned native ONNX library, the Windows import library where applicable, and upstream LICENSE, ThirdPartyNotices.txt and VERSION_NUMBER. ONNX remains optional for compiler-only developer builds. CPU release builds require it and reject a missing SDK.

| Target | Native CI runner | Required external toolchain | Candidate CPU execution |
| --- | --- | --- | --- |
| Linux x64 | Ubuntu 24.04 | LLVM/Clang/MLIR 18, CMake, Ninja, OpenSSL 3 | Installed interpreter and static/shared C ABI; existing Linux MLIR source gate retained |
| Linux ARM64 | Ubuntu 24.04 ARM64 | LLVM/Clang 18, CMake, Ninja, OpenSSL 3 | Installed interpreter and static/shared C ABI |
| macOS ARM64 | macOS 15 Apple Silicon | Homebrew LLVM 18, CMake, Ninja, OpenSSL 3 | Installed interpreter and static/shared C ABI |
| Windows x64 | Windows Server 2025 | MSYS2 UCRT64 Clang/LLVM 22.1.8-3, CMake, Ninja, OpenSSL 3 | Installed interpreter and static/shared C ABI |

The archive bundles ONNX, not the complete LLVM toolchain or all operating-system/compiler dependencies. The tests run on clean hosted machines with the prerequisites above installed. On Windows, deploy the bundled ONNX DLL and any shared ShortHand DLLs beside each consuming executable. Adding the package `bin` directory to PATH alone is insufficient: Windows can select an older ONNX in its system directory first. The installed CLIs already share their directory with the bundled ONNX DLL. No Python interpreter is required by the inference executable. Python is used only by qualification scripts.

Windows also requires the [Visual C++ 2019 runtime required by ONNX Runtime](https://onnxruntime.ai/docs/install/). Hosted Windows images already include this dependency; the archive does not install it. A minimal end-user Windows installation still needs a separate clean-machine rehearsal with the documented prerequisites.

Both Windows CI lanes install the six mutually dependent LLVM 22.1.8-3 packages with `scripts/install_ci_windows_llvm.sh`. Their official MSYS2 archive URLs and SHA-256 hashes are pinned; each detached signature must verify against the MSYS2 keyring before installation. The transaction requires `LocalFileSigLevel = Required TrustedOnly` in a temporary pacman configuration. This prevents a rolling repository update from silently changing the qualification toolchain. Other external dependencies still follow the documented package repositories.

All four ONNX SDK archive hashes are pinned in `scripts/install_ci_onnxruntime_cpu.sh`, taken from the [official ONNX Runtime 1.30.0 release asset metadata](https://api.github.com/repos/microsoft/onnxruntime/releases/tags/v1.30.0). Hashes are checked before extraction. CPU packages preserve the shared library's platform naming and installation-relative lookup path. CMake consumers use `find_package(ShortHand CONFIG REQUIRED)` and `ShortHand::runtime` or `ShortHand::runtime_shared`.

For a Windows consumer using CMake 3.21 or newer, copy the runtime DLLs recorded in those imported targets after linking:

```cmake
find_package(ShortHand CONFIG REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ShortHand::runtime)
if(WIN32)
  add_custom_command(TARGET my_app POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "$<TARGET_RUNTIME_DLLS:my_app>" "$<TARGET_FILE_DIR:my_app>"
    COMMAND_EXPAND_LISTS VERBATIM)
endif()
```

This follows [CMake's runtime DLL deployment mechanism](https://cmake.org/cmake/help/v3.21/manual/cmake-generator-expressions.7.html#genex:TARGET_RUNTIME_DLLS) and [Windows DLL search order](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order). The native package test checks the deployed ONNX hash against the archive and inspects the module actually loaded by both consumers.

## Mandatory archive test

The same `.github/workflows/cpu-packages.yml` runs for CI and release candidates. It installs both CLIs through CMake, builds the release bundle, then calls `scripts/check_installed_cpu_package.py` against that exact archive. This test:

1. Renames the build tree and downloaded SDK for the duration of qualification; restores them even on ordinary exceptions.
2. Extracts into a different directory containing spaces and removes other ONNX SDK loader paths.
3. Runs both installed CLIs and a semantic output comparison.
4. Builds installed static/shared C ABI consumers. On platforms outside the existing production scope, both must reject execution by default, preserve the output buffer, and report no successful inference. The explicit experimental mode then checks four exact ONNX identity outputs, route reuse, nonfinite rollback, reset and truthful qualification telemetry.
5. Compiles and executes a core `.short` source and separately checks real ONNX execution in the installed interpreter.
6. Deletes the bundled ONNX library in the temporary extraction and requires a new consumer configuration to fail.
7. Emits a receipt bound to the archive hash, source revision, native OS/architecture and successful checks.

The release candidate and privileged publication jobs require all four bundles and clean, matching receipts. The closeout-policy report is a separate artifact and is never interpreted as a bundle. Prerelease publication still needs protected-environment admission, cryptographic attestations and the existing release policy.

## Remaining source compilation and release gates

The C ABI probes are native C++ host programs. The core `.short` probe checks compiler installation and linking; it does not itself perform tensor inference. The installed interpreter test performs ONNX inference. These results must not be relabeled as full source-to-native tensor compilation or default production support on Windows/macOS. Promoting the runtime allowlist and documentation requires review of the resulting native evidence in the follow-up platform PR.

Full typed `.short` tensor inference currently uses the Linux-qualified LLVM/MLIR 18 lowering path. Extending and testing that source path on Windows and macOS is a required subsequent implementation slice before public compiled-AI parity is claimed. Windows uses a different currently qualified LLVM toolchain, so merely enabling the Linux MLIR flag is insufficient. Unsupported Intel macOS, Windows ARM64 and accelerators require separate work.

Retain native hosted results for the exact candidate, complete installed user examples and compatibility documentation, and finish the release governance and operational gates before publishing. Reduced RSS or a successful inference test is not energy evidence and does not establish a certification claim.
