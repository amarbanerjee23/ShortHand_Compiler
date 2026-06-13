#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "green_ai_tool.py"
EX = ROOT / "examples" / "green_ai"
EXAMPLES = [
    "image_classification.greenai",
    "llm_rag.greenai",
    "training_pipeline.greenai",
    "model_routing.greenai",
    "ai_infer_sidecar.greenai",
]


def run(*args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run([str(TOOL), *args], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if check and proc.returncode != 0:
        raise AssertionError(f"command failed: {args}\nstdout={proc.stdout}\nstderr={proc.stderr}")
    return proc


def manifest(text: str) -> Path:
    fd, name = tempfile.mkstemp(suffix=".greenai")
    os.close(fd)
    path = Path(name)
    path.write_text(text)
    return path


def valid_min(extra: str = "", mode: str = "strict") -> str:
    return f'''green {{ green_mode: "{mode}" certification_profile: "C3-ECO-AI" functional_unit: "1000_inferences" success_criteria: {{ p95_latency_ms_max: 250 }} boundary: {{ include: ["inference"] exclude: [] }} measurement_quality: "MQ1" data_quality: "DQ1" }} carbon {{ accounting_method: "location_based" region: "X" grid_factor_kgco2e_per_kwh: 1 offsets_allowed_in_core_score: false }} budgets {{ memory_peak_mb_max: 1000 }} target t {{ hardware: "cpu" runtime: "onnxruntime" green_target: {{ measure_idle_watts: false }} }} measurements {{ functional_unit_count: 1000 memory_peak_mb: 100 }} {extra}'''


def test_validate_examples() -> None:
    for name in EXAMPLES:
        run("validate", str(EX / name), "--strict", "strict")


def test_report_schema_and_carbon_formula() -> None:
    with tempfile.TemporaryDirectory() as td:
        out = Path(td) / "report.json"
        run("report", str(EX / "image_classification.greenai"), "--output", str(out), "--strict", "strict")
        report = json.loads(out.read_text())
        assert report["report_schema_version"] == "green-ai-evidence/v1"
        assert report["functional_unit"] == "1000_inferences"
        assert report["success_criteria"]["accuracy_min"] == 0.92
        assert "include" in report["boundary"]
        assert report["measurement_quality"] == "MQ2"
        assert report["data_quality"] == "DQ2"
        assert "component_carbon" in report
        assert isinstance(report["budget_results"], list)
        assert report["derived"]["total_operational_energy_kwh"] == 0.047
        assert abs(report["derived"]["operational_carbon_gco2e"] - 22.325) < 1e-9
        assert abs(report["derived"]["carbon_per_functional_unit_gco2e"] - 0.022325) < 1e-9
        assert report["certification_note"] == "Evidence report only; this tool does not grant certification."


def assert_invalid(text: str, expected: str) -> None:
    path = manifest(text)
    proc = run("validate", str(path), "--strict", "strict", check=False)
    assert proc.returncode == 1
    assert expected in proc.stderr


def test_strict_required_fields_and_quality_validation() -> None:
    assert_invalid('green { green_mode: "strict" success_criteria: { ok: true } boundary: { include: ["inference"] } measurement_quality: "MQ1" data_quality: "DQ1" } carbon { accounting_method: "location_based" region: "X" grid_factor_kgco2e_per_kwh: 1 offsets_allowed_in_core_score: false } budgets { memory_peak_mb_max: 1 }', "functional_unit")
    assert_invalid('green { green_mode: "strict" functional_unit: "1_unit" boundary: { include: ["inference"] } measurement_quality: "MQ1" data_quality: "DQ1" } carbon { accounting_method: "location_based" region: "X" grid_factor_kgco2e_per_kwh: 1 offsets_allowed_in_core_score: false } budgets { memory_peak_mb_max: 1 }', "success_criteria")
    assert_invalid('green { green_mode: "strict" functional_unit: "1_unit" success_criteria: { ok: true } measurement_quality: "MQ1" data_quality: "DQ1" } carbon { accounting_method: "location_based" region: "X" grid_factor_kgco2e_per_kwh: 1 offsets_allowed_in_core_score: false } budgets { memory_peak_mb_max: 1 }', "boundary")
    assert_invalid('green { green_mode: "strict" functional_unit: "1_unit" success_criteria: { ok: true } boundary: { include: ["inference"] } measurement_quality: "MQ1" data_quality: "DQ1" } carbon { accounting_method: "location_based" grid_factor_kgco2e_per_kwh: 1 offsets_allowed_in_core_score: false } budgets { memory_peak_mb_max: 1 }', "carbon.region")
    assert_invalid('green { green_mode: "strict" functional_unit: "1_unit" success_criteria: { ok: true } boundary: { include: ["inference"] } measurement_quality: "MQ1" data_quality: "DQ1" } carbon { accounting_method: "location_based" region: "X" offsets_allowed_in_core_score: false } budgets { memory_peak_mb_max: 1 }', "grid_factor")
    assert_invalid(valid_min().replace('offsets_allowed_in_core_score: false', 'offsets_allowed_in_core_score: true'), "offsets")
    assert_invalid(valid_min().replace('measurement_quality: "MQ1"', 'measurement_quality: "BAD"'), "measurement_quality")
    assert_invalid(valid_min().replace('data_quality: "DQ1"', 'data_quality: "BAD"'), "data_quality")


def test_block_specific_validation() -> None:
    assert_invalid(valid_min('train model m { dataset: "d" }'), "green_train")
    assert_invalid(valid_min('infer endpoint e { model: "m" }'), "green_infer")
    assert_invalid(valid_min('llm task q { model: "m" max_input_tokens: 10 max_output_tokens: 10 green_llm: { prefer_small_model: true } }'), "token_budget_per_task")
    assert_invalid(valid_min('rag pipeline r { green_rag: { embedding_cache_enabled: true } }'), "retrieve_top_k_max")
    assert_invalid(valid_min('model big { green_model: { model_size_mb: 600 smaller_model_evaluated: false } }').replace('target t { hardware: "cpu" runtime: "onnxruntime" green_target: { measure_idle_watts: false } }', ''), "deployment target")
    assert_invalid(valid_min('model big { green_model: { model_size_mb: 600 smaller_model_evaluated: false } }'), "model_necessity_justification")


def test_budget_statuses_unknown_and_not_evaluated() -> None:
    text = valid_min().replace('budgets { memory_peak_mb_max: 1000 }', 'budgets { memory_peak_mb_max: 1000 p99_latency_ms_max: 1 custom_budget_max: 7 }')
    report = json.loads(run("report", str(manifest(text)), "--strict", "strict", check=False).stdout)
    statuses = {r["budget"]: r["status"] for r in report["budget_results"]}
    assert statuses["memory_peak_mb_max"] == "pass"
    assert statuses["p99_latency_ms_max"] == "not_evaluated"
    assert statuses["custom_budget_max"] == "unknown"
    assert any("unknown budget" in w for w in report["warnings"])


def test_budget_failure_and_additive_carbon_and_offsets_not_subtracted() -> None:
    text = valid_min().replace('memory_peak_mb: 100', 'memory_peak_mb: 2000 embodied_carbon_gco2e: 5 network_carbon_gco2e: 2').replace('memory_peak_mb_max: 1000', 'memory_peak_mb_max: 1000')
    report = json.loads(run("report", str(manifest(text)), "--strict", "strict", check=False).stdout)
    mem = next(r for r in report["budget_results"] if r["budget"] == "memory_peak_mb_max")
    assert mem["status"] == "fail"
    assert report["component_carbon"]["additive_carbon_gco2e"]["embodied_carbon_gco2e"] == 5.0
    assert report["derived"]["total_carbon_gco2e"] == 7.0
    assert report["offsets_reported_separately"] == 0


def test_missing_grid_factor_not_valid_evidence_in_advisory() -> None:
    text = valid_min(mode="advisory").replace('grid_factor_kgco2e_per_kwh: 1 ', '')
    report = json.loads(run("report", str(manifest(text)), "--strict", "advisory", check=False).stdout)
    assert report["derived"]["operational_carbon_gco2e"] is None
    assert any("grid_factor" in w for w in report["warnings"])


def test_green_mode_off_and_advisory_behavior() -> None:
    off = manifest('green { green_mode: "off" }')
    assert run("validate", str(off), "--strict", "off").returncode == 0
    advisory = manifest('green { green_mode: "advisory" }')
    proc = run("validate", str(advisory), "--strict", "advisory")
    assert proc.returncode == 0
    assert "warning:" in proc.stderr


def test_eco_regression_detection_and_custom_threshold() -> None:
    with tempfile.TemporaryDirectory() as td:
        baseline = Path(td) / "baseline.json"
        baseline.write_text(json.dumps({"derived": {"energy_per_functional_unit_kwh": 0.000001, "carbon_per_functional_unit_gco2e": 0.01, "memory_peak_mb": 100, "tokens_per_task": 100, "model_size_mb": 10}}))
        proc = run("check", str(EX / "image_classification.greenai"), "--baseline", str(baseline), "--threshold-percent", "10", check=False)
        assert proc.returncode == 1
        result = json.loads(proc.stdout)
        assert any(r["field"] == "carbon_per_functional_unit_gco2e" and r["remediation_hint"] for r in result["regressions"])
        proc2 = run("check", str(EX / "image_classification.greenai"), "--baseline", str(baseline), "--threshold-percent", "1000000", check=False)
        assert proc2.returncode == 0


def test_llm_rag_and_route_metadata_in_report() -> None:
    report = json.loads(run("report", str(EX / "llm_rag.greenai"), "--strict", "strict").stdout)
    assert report["llm_tasks"][0]["green_llm"]["token_budget_per_task"] == 2500
    assert report["rag_pipelines"][0]["green_rag"]["retrieve_top_k_max"] == 5
    assert report["model_routes"][0]["green_route"]["target_small_model_handling_percent"] == 70


if __name__ == "__main__":
    for name, fn in sorted(globals().items()):
        if name.startswith("test_"):
            fn()
    print("green_ai_tool tests passed")
