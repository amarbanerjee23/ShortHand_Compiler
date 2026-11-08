# Experimental native AOT execution

The standalone `ShortHand::aot_support` static library is the support layer for
explicit bounded FP32 AOT artifacts. It has no ONNX Runtime SDK or shared-runtime
dependency and does not extend the frozen public runtime ABI. The artifact-local
`sh_aot_v1_prepare` and `sh_aot_v1_commit` helpers return status 0 on success,
1 for invalid regions/counts, 2 for non-finite data, 3 for denied CPU policy and
4 for an unsupported floating-point environment.

Callers provide live aligned arrays with exact element counts (1 through 65,536).
Address overflow is rejected before data access. Workspace must be disjoint from
input and output; input and output may alias partially or completely. Native
pointer checks cannot establish whether the caller really owns a live allocation.
All input values are checked before arithmetic, and the complete workspace must
be finite before output is copied. Any failure leaves output unchanged.

Each call refreshes CPU routing policy through the existing runtime parser and
platform override. No second routing cache or mutable workspace is introduced.
The default environment needs no heap allocations or file reads in this helper;
a configured minimum-memory constraint uses the existing host-memory probe.
Environment changes must be synchronized with calls, as with the existing runtime.
Non-Linux-x64 platforms require `SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE=1`.
This does not label generated AOT kernels as ONNX Runtime or production-qualified.

The kernel contract requires round-to-nearest and gradual FP32 underflow.
Preparation rejects other rounding modes and input/output subnormal flushing.
Callers must preserve the floating-point environment and buffers during a call;
trapping floating-point environments are outside this contract.

`test_native_aot_support.py` tests size/alignment/overflow/alias rejection,
non-finite rollback, environment policy changes, rounding, four concurrent
workers, and a 1,000-call allocation check with a live positive control. Local
GCC normal and ASan/UBSan runs pass; local leak detection is disabled. Hosted
ASan/LSan/UBSan remains mandatory. Source integration is described below.
These contracts are not timing, energy or production qualification evidence.

## Source compilation modes

The SDK now exposes two explicit LLVM-IR modes (requires its MLIR component):

```sh
short_hand application.short compile-aot --output application.ll
clang++ application.ll -O2 -lshorthand_aot_support -o application
```

Use the installed CMake target `ShortHand::aot_support` when resolving the archive
and its include directory. This target does not link ONNX Runtime. The compiler
itself retains its configured SDK dependencies; the generated standalone program
can run after model files and ONNX Runtime are removed. Ordinary `run`, `compile`,
`compile-bc` and `compile-mlir` retain their previous behavior.

`compile-aot` accepts only the native importer's bounded static FP32 MatMul+Add
subset. Every declaration must request exactly `onnxruntime_cpu`, use ONNX and
match the imported rank-2 signature. This preference constrains the device; the
artifact is explicitly labeled `experimental_aot_f32_v1`, not an ORT execution.
At most 16 models, 16 MiB per snapshot and 64 MiB total snapshot data are accepted.
Compilation reads each model into one bounded owned snapshot; constants and the
full SHA-256 identity are embedded. A rejected model never falls back.

`compile-hybrid` embeds the same kernel for supported models. The intermediate
`RequiresBackendValidation` disposition uses existing frozen-ABI runtime lowering:
registration, qualified real CPU preparation, current policy, full mutable-model
content validation, checked I/O and transactional output. A valid unsupported
Identity can execute; an unknown operator cannot report success. This path still
requires the original model and the real runtime at execution. It is not an
immutable fallback artifact; model replacement retains existing runtime semantics.
Malformed or unsafe snapshots fail compilation in both modes before output is
written. Legacy string inference is outside these explicit tensor modes. Standalone
AOT excludes runtime measurement contracts; hybrid programs retain the existing
explicitly declared measurement behavior, which is not automatic execution evidence.

The generated artifact exports `__sh_aot_v1_<full_sha256>`:

```cpp
extern "C" int32_t artifact(const float *input, uint64_t input_count,
                           float *output, uint64_t output_count,
                           float *workspace, uint64_t workspace_count);
```

Its six-argument interface is artifact-local, experimental and separate from the
frozen runtime ABI. Counts are elements, workspace count must equal output count,
and the existing v1 support status values apply. C++ and ShortHand call the same
compiled object. SDK clients constructing SemanticIR descriptors own their
provenance; the verifier checks limits, finite constants, signatures and consistent
identity but cannot reconstruct an omitted original ONNX snapshot.

The kernel reduces in increasing input-index order using separate FP32 multiply
and add, then adds bias. No fast-math, FMA or reassociation flags are emitted.
Results are byte-identical to the declared scalar FP32 oracle on tested inputs;
ORT may use a different reduction and is compared with explicit tolerances.
Consumers must preserve these FP semantics when compiling the generated IR.
MatMul and Add share one transactional output workspace. Source functions reserve
one scratch array sized to their largest direct AOT site (at most 256 KiB), reused
across sequential calls and loops. Recursion gets separate frames. Exported calls
use caller-owned disjoint workspace and support concurrent calls. Existing source
tensors still have their documented program lifetime and shared recursion semantics;
this change does not make concurrent source-global mutation safe.

The mandatory source test covers native preparation atomicity/work limits and O0/O2, nonzero inputs, five shapes including the
65,536-output bound, same-object C++ byte parity, allocation instrumentation with a
positive control, complete/partial aliasing, NaN/Inf rejection, finite overflow
rollback, CPU policy rejection, repeated sites, recursion, FMA-sensitive cancellation,
malformed snapshots and valid/invalid hybrid graphs. Installed consumers at both
optimization levels require output `86 173` after model and vendor runtime removal
on all four native platform lanes. Non-Linux-x64 lanes retain their experimental
override requirement. The Python oracle disables vendor telemetry before import.
These correctness contracts do not establish a speed or energy improvement.
