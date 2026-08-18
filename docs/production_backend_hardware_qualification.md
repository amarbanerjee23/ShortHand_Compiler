# Production backend and hardware qualification

backend_hardware_qualification_version: shorthand.backend_hardware_qualification.v1
production_scope: linux-x64-cpu-v1
production_supported_backend: onnxruntime_cpu
production_supported_device_class: cpu
accelerator_support_status: not_production_supported_until_live_qualified
production_claim_boundary: production_scope_is_versioned_and_does_not_imply_accelerator_support

## Purpose

This contract closes roadmap PR80 for a deliberately narrow production backend scope. ShortHand must never convert hardware detection, SDK presence, policy compatibility or an unavailable-path test into a production execution claim.

The v1 production-supported inference pair is:

- backend: `onnxruntime_cpu`
- device class: `cpu`
- platform qualification lane: Linux x86_64
- model fixture: `identity_float32_v13.onnx`
- input: float32 value `42`
- required output: float32 value `42`
- permitted result: real `Success` from `onnxruntime_cpu` with no fallback or not-executed status

This does not make every ONNX model or every host production-qualified. It establishes the backend/device execution contract and a mandatory numerical reference fixture. Model-specific correctness, application quality and later performance/energy budgets remain separate evidence.

## Runtime qualification enforcement

`ProductionBackendQualification.h` sits after hardware discovery/routing and before backend execution. A backend may be detected, available and compatible but still be rejected for the production path when its backend/device pair lacks live qualification.

Default behavior is fail closed:

- `onnxruntime_cpu` on CPU is allowed by the v1 production support policy.
- GPU routes such as `onnxruntime_cuda`, `onnxruntime_tensorrt` and `tensorrt` are rejected from the production route until real GPU numerical fixtures exist.
- OpenVINO NPU execution is not production-supported until a real NPU fixture exists.
- LibTorch and llama.cpp accelerator routes are not production-supported until live device-backed fixtures exist.
- TPU is inventoried but has no production backend in v1 and cannot be selected as a production route.

Rejected routes publish `backend_device_not_production_qualified` and structured evidence rather than silently falling through as successful accelerator execution.

## Experimental override

`SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE=1` permits an explicitly experimental backend/device route when the existing backend reports itself execution-ready. The selection evidence records `experimental_override:true` and `production_qualified:false`.

The override is not production evidence and cannot satisfy the mandatory qualification gate. It exists so future backend/device development can continue without weakening the default production claim boundary.

## Mandatory live CPU qualification

`scripts/install_ci_onnxruntime_cpu.sh` acquires the fixed ONNX Runtime `1.20.1` Linux x64 release archive and verifies its pinned SHA-256 before extraction. CI does not accept an unversioned or checksum-unverified SDK for this gate.

`scripts/check_production_backend_hardware_qualification.sh` then requires `ONNXRUNTIME_ROOT` and fails if it is absent. It executes the existing compiled C ABI inference fixture, requires the identity output `42`, rejects skip/fallback/not-executed evidence and verifies that the backend matrix records `onnxruntime_cpu` as `live_success`.

The resulting report is `/tmp/shorthand_production_backend_hardware_qualification.json` with schema `shorthand.backend_hardware_qualification.v1` and `mandatory_skips:0` for the declared production scope.

## Hardware support matrix

| Device class | Backend | v1 production status | Required evidence to expand support |
| --- | --- | --- | --- |
| CPU | `onnxruntime_cpu` | Production-supported by mandatory Linux x64 live qualification | Additional platform/model rows require their own numerical qualification. |
| GPU | `onnxruntime_cuda` | Not production-supported | Real GPU device, provider confirmation and numerical output fixture. |
| GPU | `onnxruntime_tensorrt` | Not production-supported | Real GPU device, TensorRT EP confirmation and numerical output fixture. |
| GPU | `tensorrt` | Not production-supported | Real GPU TensorRT engine execution and numerical output fixture. |
| NPU | `openvino` | Not production-supported | Real accessible NPU and numerical OpenVINO execution fixture. |
| GPU | `libtorch` | Not production-supported | Real GPU execution fixture with numerical validation. |
| GPU | `llamacpp` | Not production-supported | Real GPU execution fixture with deterministic model/output validation. |
| TPU | none | Not production-supported | A supported TPU backend plus real device execution fixture. |

Hardware inventory still records CPU/GPU/TPU/NPU classes. Absence from the production-supported matrix does not hide hardware. It prevents unsupported hardware from being marketed as qualified execution.

## Test layers

- Contract/unit: `tests/integration/test_production_backend_hardware_qualification.sh` proves CPU allow, accelerator deny, structured evidence and explicit experimental override behavior.
- Positive integration: `tests/integration/test_compiled_hook_onnxruntime_success.sh` performs real ONNX Runtime CPU inference and numerical output validation.
- Negative integration: unqualified GPU/NPU/TPU routes fail closed by default; existing backend unavailable-path fixtures remain mandatory claim-safety evidence.
- Regression: the gate rejects the prior state where optional SDK absence could make the backend matrix pass without live production execution.
- Sanitizer: existing runtime ASan/LSan/UBSan and TSan gates remain mandatory; the new policy is header-only routing logic exercised by compiler CI.
- Security: the CI SDK is pinned and checksum verified; dependency inventory records the acquisition.
- Portability: the declared v1 live production lane is Linux x64. Existing language/compiler portability remains broader and is not reduced.
- Performance/energy: no performance or lower-energy claim is introduced. Those remain roadmap PR86.

## Claim boundary

Closing TST022 for this versioned v1 contract means every backend/device pair that ShortHand currently calls production-supported has mandatory live numerical execution evidence. It does not mean GPU, TPU or NPU production support exists.

Any future change that advertises a new production backend, provider, device class or platform must add live numerical execution evidence before changing this matrix. Detection alone, an installed SDK, a mocked capability or an unavailable-path proof is insufficient.
