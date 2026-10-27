# Owned immutable model capability (internal v1)

Baseline: merged PR124, `cb25d880e76f6017d960bba14783715e87a7ea24`.
Scope: R2 runtime ownership and preparation. Source-language embedding, registration hoisting and AOT operator compilation remain separate work.

## Contract

`runtime/ImmutableModelInternal.h` defines `shorthand::runtime::immutable_v1::{bind,infer,release}` for source-controlled internal consumers of the static runtime archive. It is not installed, has hidden visibility in shared libraries and adds no exported C ABI symbols. The existing 25-symbol C ABI and mutable-path APIs retain their behavior. Rebuilding internal C++ consumers is required; no new stable C++ ABI is promised.

`bind` copies the supplied bytes before validating their SHA-256 against a required lowercase 64-character digest. The copy, its model descriptor and prepared ORT session stay owned by a private slot. Caller storage must be readable and unchanged during `bind`; it may be changed or freed after the call returns. The descriptor path is replaced with a diagnostic digest label and is never opened. A digest checks integrity against a caller-trusted value, not signatures, provenance or measurement authenticity.

The initial accepted boundary is self-contained ONNX IR 8–10, one empty-domain opset 13 import, a single FP32 input/output and concrete declared shapes. Each tensor is limited to 65,536 elements and rank 8; the model snapshot is nonempty and at most 16 MiB. ORT checks graph semantics, supported operators and declared shape compatibility. Dynamic model dimensions may be concretized by the declared shape using the existing prepared-session rules. No quantization or precision conversion is requested by this binding API.

Before preparing ORT, a bounded protobuf walk inspects dependency-bearing containers. External tensor metadata/locations, segmented tensors, custom node domains, training/functions, device configurations, unknown fields in those containers and malformed nesting reject. Tensor attributes, graph attributes, repeated attributes and sparse initializers are traversed. Depth is limited to 64 and visited fields to one million. This is a dependency-envelope check, not a replacement ONNX checker or a sandbox for arbitrary models. Nested type/value metadata without file dependencies remains ORT's validation responsibility.

## Ownership and lifecycle

There is one immutable slot per linked runtime instance. Binding, inference, release and `short_runtime_reset` share the existing recursive facade mutex. A successful bind prepares one candidate session and atomically replaces the previous slot. A failed replacement preserves the old slot. The old and candidate snapshots can coexist during replacement, and SHA-256 currently uses a temporary string copy; the 16 MiB snapshot limit is not a bound on total process or ORT memory.

Every successful bind receives a new nonzero 64-bit generation. Release and reset destroy the slot without resetting the generation counter. A stale, zero or previously released handle rejects before touching the caller's output. Generation exhaustion rejects rather than wrapping. A handle is a lifecycle token, not an authorization or secrecy boundary. Handles must not cross independently linked runtime instances or dynamic library unload/reload boundaries.

Each inference refreshes environment routing policy and checks the qualified CPU route. A warm session cannot bypass a newly denied CPU or production qualification policy. Native candidate tests outside the current `linux-x64-cpu-v1` allowlist retain the existing explicit experimental opt-in. The binding does not expand that allowlist.

Input is a synchronous read-only FP32 borrow with exact counts and valid aligned storage. The backend writes private scratch. The wrapper validates finite outputs and commits only after all potentially throwing work succeeds, allowing exact or partial input/output overlap. Failure leaves caller output unchanged. Caller pointers are not retained. Scratch belongs to the serialized slot; this does not provide parallel inference or tenant isolation. Internal results carry hardware/telemetry and artifact identity; they do not alter the public facade's named-registration counters or last-result observations.

The mutable path remains separate: it rereads complete model contents and detects same-size replacements with preserved timestamps. External-weight models keep path-based execution and dependency-change behavior. No path is silently treated as immutable.

## Evidence and limitations

The native suite checks independent Python-generated digest fixtures, source-byte/descriptor mutation after binding, 25 envelope/graph negatives, failed replacement rollback, the 16 MiB boundary, invalid I/O, partial aliasing, nonfinite input/output, policy changes, release/reset and stale generations. A four-worker test executes 2,000 calls while 32 resets and replacements occur. The existing mutable-path cache suite remains mandatory. The real runtime archive is instrumented for ASan/UBSan; LeakSanitizer availability is recorded by the existing sanitizer runner. The vendor ORT binary itself is not instrumented.

Installed-package qualification runs the new contract suite against each relocated static archive, with build and SDK paths hidden, on Linux x64/arm64, macOS arm64 and Windows x64. The test obtains its private header from the checkout, not the installed SDK. Candidate verification requires the new lifecycle receipt. The normal shared runtime is checked for private symbol leakage. Existing interpreter, generated-source, shared-library, missing-dependency and installer/uninstaller tests remain release gates.

`experiments/energy/immutable_model_evidence.py` retains 36 uninstrumented observations: three synthetic Identity model sizes (88 B, 64 KiB and 1 MiB), six balanced paired blocks, one thread, eight warmups and 2,048 checked timed calls. Both hosts retain input/output validation and private output scratch; the immutable candidate additionally includes the serialized facade. The large files use legal doc-string padding to isolate model-byte validation overhead. They are not representative large neural networks, and these internal-host timings are not generated `.short` or C++ superiority results.

Separate `strace -f -yy` runs include all threads and mark the warm region. The immutable trace must contain zero model-path I/O calls. Mutable-path and deliberately injected-read controls must both show at least 16 opens and reads, proving detection is active. Trace timings never enter latency results. Hosted CI fails when tracing is absent or a control fails. The local diagnostic flag can retain a permission-denied trace attempt, but such a report cannot pass the hosted evidence index.

Six separate component-energy executions include startup, preparation, all 2,057 verified calls and teardown. Missing counters produce null joules. Counter energy is neither process-attributed nor whole-system energy. Setup amortization, allocated bytes and energy savings are not inferred from latency. All raw observations, commands, executable/SDK snapshots, source snapshots/diff, traces and hashes are retained for replay. Hashes establish integrity relative to a trusted root digest; they do not authenticate a host. Hosted artifacts expire after 90 days unless archived.

## Reproduction

Use the verified ORT 1.30.0 SDK and build flags from `runtime-profile.yml`:

```sh
CXX=clang++-18 python3 tests/ai_application/test_immutable_model.py \
  --build build-profile --sdk build-profile/onnxruntime-1.30.0
scripts/check_prepared_cache_sanitizers.sh \
  "$PWD/build-profile/onnxruntime-1.30.0" "$PWD/build-profile-sanitized"
CXX=clang++-18 python3 experiments/energy/immutable_model_evidence.py \
  --build build-profile --sdk build-profile/onnxruntime-1.30.0 --output immutable-output
python3 experiments/energy/immutable_model_evidence.py --output immutable-output \
  --replay-sha256 "$(cat immutable-output/manifest.sha256)"
python3 experiments/energy/test_immutable_model_evidence.py --bundle immutable-output
```

The output directory must be new. Linux `strace` is required for capture; replay uses Python's standard library. Initial local observations and environmental limitations are in [the benchmark report](latency_energy_test_and_benchmark_results.md). Exact revision CI/release links belong in the PR after completion.

## Primary references

- [ORT session construction from an array](https://onnxruntime.ai/docs/api/c/struct_ort_1_1_session.html): the prepared session consumes the verified snapshot rather than reopening a path.
- [ONNX external-data security](https://onnx.ai/onnx/repo-docs/ExternalDataSecurity.html): dependency references require explicit validation; this API rejects them.
- [Repository implementation sequence](post_pr122_energy_implementation_plan.md) and [frozen runtime ABI](runtime_abi_api_stability.md).
