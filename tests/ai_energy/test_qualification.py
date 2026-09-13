#!/usr/bin/env python3
"""CPU-only deterministic integration gate. No synthetic record is qualification evidence."""
import base64
import copy
import hashlib
import json
import pathlib
import subprocess
import sys

root, tool, work, measure, live = sys.argv[1:]
root, work = pathlib.Path(root), pathlib.Path(work)
checks = 0


def run(args, success=True, reason=None):
    global checks
    result = subprocess.run([str(x) for x in args], text=True, capture_output=True, timeout=120)
    assert (result.returncode == 0) == success, (args, result.returncode, result.stdout, result.stderr)
    assert not any(marker in result.stderr for marker in ("AddressSanitizer", "LeakSanitizer", "runtime error:", "ThreadSanitizer")), result.stderr
    if reason:
        assert reason in result.stdout + result.stderr, (reason, result.stdout, result.stderr)
    checks += 1
    return result


def write(name, value):
    p = work / name
    p.write_text(json.dumps(value, sort_keys=True) + "\n")
    return p


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


run([tool, "probe"])
training = {"schema": "shorthand.ai.cpu_qualification.config.v1", "mode": "training", "workload": "reference_cnn_test",
            "threads": [1], "warmups": 1, "repetitions": 1, "trials": 2, "energy_source": "unavailable"}
config = write("training.json", training)
report = work / "training-report.json"
run([tool, "qualify", config, report], reason="cpu_baseline_without_energy_optimization")
r = json.loads(report.read_text())
assert r["selected_candidate"] == 0 and not r["comparative_energy_claim"]
assert not r["official_certification"] and not r["lowest_carbon_language_claim"]
assert not r["qualification_total_energy"]["available"]
t = r["candidates"][0]["training"]
assert len(t) == 2 and all(x["success"] and x["validation_accuracy"] >= .95 and x["final_loss"] < x["initial_loss"] for x in t)
assert all(x["samples_processed"] == 1024 and x["optimizer_steps"] == 128 and len(x["epochs"]) == 16 and len(x["steps"]) == 128 for x in t)
assert t[0]["parameters_sha256"] == t[1]["parameters_sha256"]
run([tool, "execute", config, report, sha(report), work / "trained.json"])
run([tool, "execute", config, report, "0" * 64, work / "tampered.json"], False, "report_digest_mismatch")
for field, value, reason in [("hardware_sha256", "0" * 64, "stale_or_incompatible_profile"),
                             ("compiler_revision", "stale", "stale_or_incompatible_profile"),
                             ("model_sha256", "0" * 64, "stale_or_incompatible_profile"),
                             ("selected_candidate", None, "profile_has_no_selected_candidate")]:
    bad = copy.deepcopy(r)
    bad[field] = value
    bad_path = write("stale.json", bad)
    run([tool, "execute", config, bad_path, sha(bad_path), work / "stale-result.json"], False, reason)
for patch, reason in [({"precision": "int8"}, "invalid_workload_or_precision_contract"),
                      ({"trials": 1}, "qualification_protocol_limit"),
                      ({"repetitions": 2}, "training_repeats_are_counted_by_trials"),
                      ({"threads": [0]}, "invalid_thread_count"),
                      ({"threads": [1.5]}, "invalid_thread_count"),
                      ({"energy_source": "synthetic_test"}, "unsupported_energy_source"),
                      ({"typo_option": True}, "unknown_configuration_key"),
                      ({"quality_metric": "top1_agreement"}, "training_requires_validation_accuracy"),
                      ({"batch_size": 0}, "qualification_protocol_limit")]:
    run([tool, "qualify", write("invalid.json", dict(training, **patch)), work / "invalid-report.json"], False, reason)
strict = dict(training, require_measured_energy=True)
run([tool, "qualify", write("strict.json", strict), work / "strict-report.json"], False, "required_measured_energy_unavailable")
assert json.loads((work / "strict-report.json").read_text())["selected_candidate"] is None
bad_training = dict(training, quality_threshold=1, training={"epochs": 1, "learning_rate": 1e-7})
run([tool, "qualify", write("quality-fail.json", bad_training), work / "failed.json"], False, "cpu_baseline_failed")
assert json.loads((work / "failed.json").read_text())["candidates"][0]["trials"][0]["reason"]
# Parsing is shared with C3-ECO: duplicate keys and non-finite numbers are forbidden.
(work / "invalid.json").write_text('{"schema":"x","schema":"y"}')
run([tool, "qualify", work / "invalid.json", work / "invalid-report.json"], False)
(work / "invalid.json").write_text('{"maximum_latency_ms": NaN}')
run([tool, "qualify", work / "invalid.json", work / "invalid-report.json"], False)

model = work / "identity.onnx"
model.write_bytes(base64.b64decode((root / "tests/fixtures/onnx/identity_float32_v13.onnx.b64").read_bytes()))
inference = {"schema": training["schema"], "mode": "inference", "workload": "identity_fp32", "threads": [1],
             "warmups": 2, "repetitions": 3, "trials": 3, "model_path": str(model), "model_sha256": sha(model), "energy_source": "unavailable"}
ic = write("inference.json", inference)
ir = work / "inference-report.json"
if live == "1":
    run([tool, "qualify", ic, ir], reason="cpu_baseline_without_energy_optimization")
    result = json.loads(ir.read_text())
    assert all(x["success"] and x["numerical"]["valid"] and x["numerical"]["maximum_absolute_error"] == 0 for x in result["candidates"][0]["trials"])
    output = work / "executed.json"
    run([tool, "execute", ic, ir, sha(ir), output])
    execution = json.loads(output.read_text())
    assert not execution["profile_search_repeated"] and execution["prepared_session_count"] == 1
    assert len(execution["output_sha256"]) == 3 and len(set(execution["output_sha256"])) == 1
    stale = copy.deepcopy(result)
    stale["candidates"][0]["backend_version"] = "incompatible"
    stale_path = write("stale-ort.json", stale)
    run([tool, "execute", ic, stale_path, sha(stale_path), output], False, "profile_runtime_version_mismatch")
    for patch, reason in [({"input_shape": [1, 2]}, "cpu_baseline_failed"),
                          ({"output_shape": [999]}, "cpu_baseline_failed"),
                          ({"maximum_memory_bytes": 8}, "unsafe_or_oversize_file")]:
        run([tool, "qualify", write("invalid-inference.json", dict(inference, **patch)), ir], False, reason)
else:
    run([tool, "qualify", ic, ir], False, "cpu_baseline_failed")
    assert "onnxruntime" in json.dumps(json.loads(ir.read_text()))
for patch, reason in [({"model_sha256": "0" * 64}, "model_sha256_mismatch"),
                      ({"model_path": str(work / "missing.onnx")}, "unsafe_or_oversize_file"),
                      ({"input_shape": [1, 99999999999]}, "invalid_shape_dimension"),
                      ({"quality_metric": "validation_accuracy"}, "invalid_inference_quality_or_training_config")]:
    run([tool, "qualify", write("invalid-inference.json", dict(inference, **patch)), ir], False, reason)

# Bind external weight contents to the configuration; changing the file must fail
# before ONNX Runtime execution, even if the protobuf graph is unchanged.
from onnx_fixture import external_model, write_weights
ext = work / "external.onnx"
external_model(ext)
weights = work / "weights.bin"
weight_hash = write_weights(weights, 12)
external_config = dict(inference, model_path=str(ext), model_sha256=sha(ext), input_shape=[1, 4], output_shape=[1, 3],
                       external_files=[{"name": "weights.bin", "sha256": weight_hash, "size_bytes": 48}])
ec = write("external.json", external_config)
if live == "1":
    run([tool, "qualify", ec, work / "external-report.json"])
else:
    run([tool, "qualify", ec, work / "external-report.json"], False, "cpu_baseline_failed")
missing_manifest = dict(external_config)
del missing_manifest["external_files"]
run([tool, "qualify", write("missing-manifest.json", missing_manifest), ir], False, "external_weights_manifest_required")
weights.write_bytes(b"x" * 48)
run([tool, "qualify", ec, ir], False, "external_weights_sha256_mismatch")
external_model(ext, location="../escape.bin")
unsafe = dict(inference, model_path=str(ext), model_sha256=sha(ext))
run([tool, "qualify", write("escape.json", unsafe), ir], False, "unsafe_onnx_external_location")
ext.write_bytes(b"\x3a\xff\xff\xff")
unsafe["model_sha256"] = sha(ext)
run([tool, "qualify", write("truncated.json", unsafe), ir], False, "invalid_or_oversize_onnx_envelope")

accounting = {"record_id": "native-qualification-test", "component": "cpu", "allocation_fraction": 1, "pue": 1,
              "carbon_factor_gco2e_per_kwh": 100, "factor_source": "test-only-factor", "factor_date": "2026-01-01",
              "tariff_per_kwh": .2, "tariff_currency": "GBP", "tariff_source": "test-only-tariff",
              "measurement_quality": "high", "data_quality": "high", "evidence_ref": "test-fixture-not-production"}
ac = write("accounting.json", accounting)
run([tool, "export-workbook", report, sha(report), ac, work / "unmeasured.tsv"], False, "measured_qualified_energy_required_for_export")
# Fabricated values below exercise serialization/accounting ONLY, never hardware qualification.
fixture = copy.deepcopy(r)
fixture["qualification_total_energy"] = {
    "available": True, "claim_eligible": True, "evidence_class": "measured", "source_kind": "physical_meter", "joules": 3600,
    "end_unix_seconds": 1780000000, "instrument": {"id": "test-meter", "calibration_id": "test-calibration", "calibration_date": "2026-01-01", "uncertainty_percent": 2}}
fp = write("serialization-only-fixture.json", fixture)
tsv, csv, workbook = work / "fixture.tsv", work / "fixture.csv", work / "fixture-workbook.json"
run([tool, "export-workbook", fp, sha(fp), ac, tsv])
run([measure, tsv, csv, workbook])
assert len(tsv.read_text().splitlines()) == 2  # No overlapping trial+total double count.
assert json.loads(workbook.read_text())["schema"] == "shorthand.c3eco.measurement_workbook.v1"
bad = dict(accounting, pue=0)
run([tool, "export-workbook", fp, sha(fp), write("bad-accounting.json", bad), tsv])
run([measure, tsv, csv, workbook], False)
bad = dict(accounting, evidence_ref="bad\trow")
run([tool, "export-workbook", fp, sha(fp), write("bad-accounting.json", bad), tsv], False, "unsafe_text")
fixture["synthetic_energy_test"] = True
fp = write("synthetic.json", fixture)
run([tool, "export-workbook", fp, sha(fp), ac, tsv], False, "invalid_report_for_workbook")
print(f"PASS {checks} native qualification CLI cases; real_onnx_required={live}; simulated records are test-only")
