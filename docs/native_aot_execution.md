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
ASan/LSan/UBSan remains mandatory. Source lowering is the next integration step.
These contracts are not timing, energy or production qualification evidence.
