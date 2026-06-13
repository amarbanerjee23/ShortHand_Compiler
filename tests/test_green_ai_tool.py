#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "green_ai_tool.py"
EX = ROOT / "examples" / "green_ai"


def run(*args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run([str(TOOL), *args], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if check and proc.returncode != 0:
        raise AssertionError(f"command failed: {args}\nstdout={proc.stdout}\nstderr={proc.stderr}")
    return proc


def test_validate_examples() -> None:
    for name in ["image_classification.greenai", "llm_rag.greenai", "training_pipeline.greenai"]:
        run("validate", str(EX / name), "--strict", "strict")


def test_report_schema_and_carbon_formula() -> None:
    with tempfile.TemporaryDirectory() as td:
        out = Path(td) / "report.json"
        run("report", str(EX / "image_classification.greenai"), "--output", str(out), "--strict", "strict")
        report = json.loads(out.read_text())
        assert report["functional_unit"] == "1000_inferences"
        assert report["measurement_quality"] == "MQ2"
        assert report["data_quality"] == "DQ2"
        assert report["derived"]["total_operational_energy_kwh"] == 0.047
        assert abs(report["derived"]["operational_carbon_gco2e"] - 22.325) < 1e-9
        assert "budget_status" in report
        assert "certification_note" in report


def test_strict_missing_functional_unit_fails() -> None:
    with tempfile.TemporaryDirectory() as td:
        manifest = Path(td) / "bad.greenai"
        manifest.write_text('green { green_mode: "strict" boundary: { include: ["inference"] } measurement_quality: "MQ1" data_quality: "DQ1" } carbon { region: "X" grid_factor_kgco2e_per_kwh: 1 offsets_allowed_in_core_score: false }')
        proc = run("validate", str(manifest), "--strict", "strict", check=False)
        assert proc.returncode == 1
        assert "functional_unit" in proc.stderr


def test_offsets_core_score_fails() -> None:
    with tempfile.TemporaryDirectory() as td:
        manifest = Path(td) / "bad_offsets.greenai"
        manifest.write_text('green { green_mode: "strict" functional_unit: "1_training_run" boundary: { include: ["training"] } measurement_quality: "MQ1" data_quality: "DQ1" } carbon { region: "X" grid_factor_kgco2e_per_kwh: 1 offsets_allowed_in_core_score: true }')
        proc = run("validate", str(manifest), "--strict", "strict", check=False)
        assert proc.returncode == 1
        assert "offsets" in proc.stderr


def test_eco_regression_detection() -> None:
    with tempfile.TemporaryDirectory() as td:
        baseline = Path(td) / "baseline.json"
        baseline.write_text(json.dumps({"derived": {"carbon_per_functional_unit_gco2e": 0.01, "memory_peak_mb": 100}}))
        proc = run("check", str(EX / "image_classification.greenai"), "--baseline", str(baseline), "--threshold-percent", "10", check=False)
        assert proc.returncode == 1
        result = json.loads(proc.stdout)
        assert not result["passed"]
        assert any(r["field"] == "carbon_per_functional_unit_gco2e" for r in result["regressions"])


def test_llm_token_budget_and_route_metadata_in_report() -> None:
    report = json.loads(run("report", str(EX / "llm_rag.greenai"), "--strict", "strict").stdout)
    assert report["llm_tasks"][0]["green_llm"]["token_budget_per_task"] == 2500
    assert report["model_routes"][0]["green_route"]["target_small_model_handling_percent"] == 70


if __name__ == "__main__":
    for name, fn in sorted(globals().items()):
        if name.startswith("test_"):
            fn()
    print("green_ai_tool tests passed")
