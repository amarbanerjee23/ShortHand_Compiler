# CPU package qualification

These are controlled-beta CPU packages. Successful trusted push builds publish development prereleases through the channel below; these are not GA-qualified releases. CPU inference on Windows x64 and macOS ARM64 is required alongside Linux. A green compiler portability check alone does not qualify an AI package. The existing default production scope remains Linux x64. On the other three targets, qualification first verifies default refusal, then explicitly sets `SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE=1` for native experiments. Receipts identify `experimental_native_candidate`; they do not promote production support.

## Package contents and prerequisites

`cmake --install` installs `short_hand`, `green_ai_tool`, the existing SDK tools and libraries, CMake targets and pkg-config metadata. With ONNX enabled, the package also includes the pinned native ONNX library, the Windows import library where applicable, and upstream LICENSE, ThirdPartyNotices.txt and VERSION_NUMBER. ONNX remains optional for compiler-only developer builds. CPU release builds require it and reject a missing SDK.

| Target | Native CI runner | Required external toolchain | Candidate CPU execution |
| --- | --- | --- | --- |
| Linux x64 | Ubuntu 24.04 | LLVM/Clang/MLIR 18, CMake, Ninja, OpenSSL 3 | Installed interpreter and static/shared C ABI; existing Linux MLIR source gate retained |
| Linux ARM64 | Ubuntu 24.04 ARM64 | LLVM/Clang/MLIR 18, CMake, Ninja, OpenSSL 3 | Installed interpreter and static/shared C ABI |
| macOS ARM64 | macOS 15 Apple Silicon | Homebrew LLVM/MLIR 18, CMake, Ninja, OpenSSL 3 | Installed interpreter and static/shared C ABI |
| Windows x64 | Windows Server 2025 | MSYS2 UCRT64 Clang/LLVM 22.1.8-3 and MLIR 22.1.8-1, CMake, Ninja, OpenSSL 3 | Installed interpreter and static/shared C ABI |

The archive bundles ONNX, not the complete LLVM toolchain or all operating-system/compiler dependencies. The tests run on clean hosted machines with the prerequisites above installed. On Windows, deploy the bundled ONNX DLL and any shared ShortHand DLLs beside each consuming executable. Adding the package `bin` directory to PATH alone is insufficient: Windows can select an older ONNX in its system directory first. The installed CLIs already share their directory with the bundled ONNX DLL. No Python interpreter is required by the inference executable. Python is used only by qualification scripts.

Windows also requires the [Visual C++ 2019 runtime required by ONNX Runtime](https://onnxruntime.ai/docs/install/). Hosted Windows images already include this dependency; the archive does not install it. A minimal end-user Windows installation still needs a separate clean-machine rehearsal with the documented prerequisites.

Both Windows CI lanes install the six mutually dependent LLVM 22.1.8-3 packages with `scripts/install_ci_windows_llvm.sh`. Their official MSYS2 archive URLs and SHA-256 hashes are pinned; each detached signature must verify against the MSYS2 keyring before installation. The transaction requires `LocalFileSigLevel = Required TrustedOnly` in a temporary pacman configuration. This prevents a rolling repository update from silently changing the qualification toolchain. Other external dependencies still follow the documented package repositories.

Windows MLIR builds and consumers use the pinned LLD linker through Clang's `-fuse-ld=lld`. The exported `ShortHandMLIR::Dialect` target propagates this requirement. This matches the [MSYS2 MLIR build configuration](https://github.com/msys2/MINGW-packages/blob/master/mingw-w64-mlir/PKGBUILD); GNU ld failed to link the pinned MLIR LLVM dialect archive in the native package lane.

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

The release candidate and privileged publication jobs require all four bundles and clean, matching receipts. The closeout-policy report is a separate artifact and is never interpreted as a bundle. Versioned `v*` prerelease publication still needs protected-environment admission, cryptographic attestations and the existing release policy. The development build channel below is separately authorized and explicitly unsigned.

## Native source compilation

All four package builds now enable MLIR. The installed `compile-mlir` command emits LLVM IR for the native host triple. Darwin uses a Mach-O segment/section pair for retained model metadata; Windows diagnostics use the `_write` CRT ABI. Linux retains its existing ELF section and `write` ABI. LLVM/MLIR 18 and 22 are accepted; the installed MLIR SDK still requires its exact build-time LLVM version.

The installed source fixture declares statically shaped float32 tensors, executes two real ONNX identity inferences through `short_ai_infer_f32`, then prints a completion marker. Separate interpreter and C ABI checks verify numeric input/output 42. Native executables must pass at `-O0` and `-O2`. Missing models must stop execution with `SHD7030`; unsupported production platforms must reject by default before an explicit experimental run. The source grammar currently zero-initializes declared tensors. Arbitrary tensor values and reading tensor outputs are available through the SemanticIR API, not the current source grammar; this test does not claim new source syntax or arbitrary tensor-input parity.

## Installable development builds

Every successful trusted branch **push** through `ci.yml` publishes one immutable development prerelease named `build-<run-id>-<attempt>-<commit>`. Pull requests, forks, failed/incomplete builds and superseded cancelled runs do not publish. The release job waits for the full mandatory CI aggregate, revalidates all four CPU and installer receipts, uploads a draft, downloads and hashes every asset, then publishes it with `latest=false`. A failed upload verification rolls back its own draft. Versioned production releases retain the protected release workflow and add provenance attestations for the installers.

| System | Installable | Default destination | Install / uninstall |
| --- | --- | --- | --- |
| Windows x64 | `*-windows-x64-setup.exe` | `%LOCALAPPDATA%\Programs\ShortHand` | Run the installer; remove using Windows Apps or `uninstall.exe` |
| Ubuntu 24.04 x64 | `*-linux-x64-setup.deb` | `/opt/shorthand` | `sudo apt install ./<file>.deb`; `sudo apt remove shorthand` |
| Ubuntu 24.04 ARM64 | `*-linux-arm64-setup.deb` | `/opt/shorthand` | Same commands with the ARM64 package |
| macOS 15 ARM64 | `*-macos-arm64-setup.pkg` | `/opt/shorthand` | Open the package or use `sudo installer -pkg <file>.pkg -target /` |

Install the external toolchain listed above first. On Linux/macOS add `/opt/shorthand/bin` to PATH; on Windows use MSYS2 UCRT64 with the installed ShortHand `bin` directory on PATH. The Windows installer is per-user and preserves unrecognized files during uninstall. Uninstall an earlier Windows installation before installing another build. For macOS removal, delete only the dedicated `/opt/shorthand` installation after moving any personal files out, then run `sudo pkgutil --forget org.shorthand.compiler`.

These development installers are **unsigned** (no Windows Authenticode or macOS Developer ID/notarization) and are not GA. SHA-256 checksums and hash-bound test receipts accompany each installer. OS trust prompts may apply. Dependencies are not silently downloaded by the installer. Intel macOS, Windows ARM64, Linux distributions other than the qualified Ubuntu image and accelerators remain outside this package matrix.

CI builds each installer from the exact verified archive, installs it using the native package mechanism, compares every installed file with the archive, runs the complete installed CPU/source suite again, and uninstalls it. The receipt is written only after all four phases pass. Qualification runs on disposable hosted machines with prerequisites installed; a minimal end-user machine rehearsal remains a separate release gate. The runtime production allowlist and energy/certification claims are unchanged.
