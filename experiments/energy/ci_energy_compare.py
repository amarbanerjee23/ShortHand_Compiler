#!/usr/bin/env python3
"""Five-cell CI component-energy comparison for ShortHand, C++/ORT and Python/ORT.

This is a separate instrumented pass from latency measurement. When readable
Linux CPU component-energy counters are unavailable it writes an explicit
unavailable artifact and exits successfully. E1 values may come from Linux
powercap/RAPL package counters or AMD amd_energy HWMON socket counters. They
cover the whole process window; they are not whole-system AC energy and are not
process-attributed.
"""
from __future__ import annotations

import argparse
import itertools
import json
import math
import os
import pathlib
import random
import statistics
import subprocess
import sys
import time
from typing import Dict, List

import campaign
import ci_energy_evidence as evidence
import runtime_state_of_practice as cpp
from compare_ai_application_baselines import validate_pair

ROOT = pathlib.Path(__file__).resolve().parents[2]
CELLS = ((1, 1), (16, 1), (32, 1), (16, 2), (16, 4))
RUNNERS = ("native", "cpp_onnx", "python_onnx")
SCHEMA = "shorthand.energy.ci_component_comparison.v1"
COMPLETED = 1797 * 2 * 3
CLAIMS = dict(campaign.CLAIMS, production_claim=False)


def write(path: pathlib.Path, value) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n")


def balanced_orders(seed: int) -> List[List[str]]:
    rows = [list(p) for p in itertools.permutations(RUNNERS)]
    random.Random(seed).shuffle(rows)
    return rows


def validate_classification(report):
    scores = report.get("scores")
    predictions = report.get("predictions")
    top = report.get("top_k")
    if not isinstance(scores, list) or len(scores) != 17970:
        raise ValueError("incomplete scores")
    if not isinstance(predictions, list) or len(predictions) != 1797:
        raise ValueError("incomplete predictions")
    if not isinstance(top, list) or len(top) != 5391:
        raise ValueError("incomplete top-k")
    if any(type(v) not in (int, float) or not math.isfinite(v) for v in scores):
        raise ValueError("nonfinite score")
    expected = []
    for offset in range(0, len(scores), 10):
        expected.extend(sorted(range(10), key=lambda n: (-scores[offset + n], n))[:3])
    if top != expected or predictions != expected[::3]:
        raise ValueError("classification ordering mismatch")


def measured_process(argv, directory: pathlib.Path, name: str) -> Dict[str, object]:
    stdout_path = directory / (name + ".stdout")
    stderr_path = directory / (name + ".stderr")
    sampler = evidence.ComponentEnergySampler(interval_seconds=0.01)
    start_wall = time.time()
    start = time.perf_counter()
    sampler.start()
    try:
        with stdout_path.open("wb") as stdout, stderr_path.open("wb") as stderr:
            proc = subprocess.run(
                list(map(str, argv)),
                stdin=subprocess.DEVNULL,
                stdout=stdout,
                stderr=stderr,
                env=dict(os.environ, **campaign.ENV),
                check=False,
                timeout=300,
            )
    finally:
        joules, samples = sampler.stop()
    elapsed = time.perf_counter() - start
    end_wall = time.time()
    if proc.returncode:
        stderr = stderr_path.read_text(errors="replace")
        raise ValueError(f"energy runner failed: {name}, exit={proc.returncode}: {stderr[-4000:]}")
    if not math.isfinite(joules) or joules <= 0:
        raise ValueError("nonpositive CPU component energy")
    if abs((end_wall - start_wall) - elapsed) > 0.05:
        raise ValueError("wall/monotonic clock discontinuity")
    result = {
        "schema": evidence.SCHEMA,
        "kind": "measurement",
        "evidence_class": "E1",
        "method": sampler.method,
        "available": True,
        "boundary": "whole_process_" + sampler.boundary,
        "reason": "component_energy_not_process_attributed",
        "physical_system_energy_measured": False,
        "component_energy_measured": True,
        "hardware_measured_joules": joules,
        "calibrated_joules_estimate": None,
        "analytical_joules_estimate": None,
        "joules_per_completed_correct_task": joules / COMPLETED,
        "completed_correct_tasks": COMPLETED,
        "elapsed_seconds": elapsed,
        "component_energy_sample_count": samples,
        "measurement_uncertainty_percent": None,
        "claim_authorized": False,
    }
    write(directory / (name + ".energy.json"), result)
    return result


def prepare_fixture(out: pathlib.Path, batch: int, threads: int) -> pathlib.Path:
    app_path = campaign.create(out, batch=batch, threads=threads)
    app = campaign.load(app_path)
    dataset = pathlib.Path(app["dataset_path"])
    copied = out / "dataset.csv"
    campaign.snapshot(dataset, copied)
    app["dataset_path"] = str(copied)
    campaign.write(app_path, app)
    cpp.case_inputs({"application": app_path, "id": f"b{batch}-t{threads}"})
    return app_path


def runner_commands(tool: pathlib.Path, control: pathlib.Path, python: pathlib.Path,
                    fixture: pathlib.Path, directory: pathlib.Path, batch: int, threads: int):
    app_path = fixture / "application.json"
    app = campaign.load(app_path)
    q = campaign.load(fixture / "qualification.json")
    return {
        "native": [
            tool, "application", app_path, directory / "native-report.json"
        ],
        "cpp_onnx": [
            control, q["model_path"], app["dataset_path"], batch, threads,
            2, 2, 3, directory / "cpp-trials.csv"
        ],
        "python_onnx": [
            python, ROOT / "scripts/compare_ai_application_baselines.py",
            "--python-worker", "--tool", tool, "--config", app_path,
            "--output", directory / "python-report.json"
        ],
    }


def validate_block(directory: pathlib.Path, fixture: pathlib.Path) -> None:
    app = campaign.load(fixture / "application.json")
    q = campaign.load(fixture / "qualification.json")
    native = cpp.validate_native_report(directory / "native-report.json")
    python = cpp.validate_native_report(directory / "python-report.json", native["predictions"])
    validate_classification(native)
    validate_classification(python)
    validate_pair(native, python, q)
    if native["top_k"] != python["top_k"]:
        raise ValueError("native/Python top-k mismatch")
    cpp.check_cpp_output(
        directory / "cpp_onnx.stdout",
        native["predictions"],
        sum(native["predictions"]) * 2 * 3,
    )
    cpp.check_cpp_validation(directory / "cpp-trials.csv", native)
    if campaign.sha(pathlib.Path(q["model_path"])) != q["model_sha256"]:
        raise ValueError("model identity changed during energy pass")
    if campaign.sha(pathlib.Path(app["dataset_path"])) != app["dataset_sha256"]:
        raise ValueError("dataset identity changed during energy pass")


def median_rows(observations: List[Dict[str, object]]) -> Dict[str, object]:
    values = [float(x["joules_per_completed_correct_task"]) for x in observations]
    total = [float(x["hardware_measured_joules"]) for x in observations]
    return {
        "blocks": len(values),
        "median_joules_per_completed_correct_task": statistics.median(values),
        "mean_joules_per_completed_correct_task": statistics.mean(values),
        "min_joules_per_completed_correct_task": min(values),
        "max_joules_per_completed_correct_task": max(values),
        "median_total_package_joules": statistics.median(total),
    }


def summarize(raw: Dict[str, Dict[str, List[Dict[str, object]]]], hardware) -> Dict[str, object]:
    all_observations = [obs for by_runner in raw.values() for values in by_runner.values() for obs in values]
    methods = {str(obs["method"]) for obs in all_observations}
    boundaries = {str(obs["boundary"]) for obs in all_observations}
    if len(methods) != 1 or len(boundaries) != 1:
        raise ValueError("component-energy method/boundary changed within comparison")
    method = next(iter(methods))
    boundary = next(iter(boundaries))
    cells = []
    for cell, by_runner in raw.items():
        rows = {name: median_rows(by_runner[name]) for name in RUNNERS}
        native = rows["native"]["median_joules_per_completed_correct_task"]
        cpp_j = rows["cpp_onnx"]["median_joules_per_completed_correct_task"]
        py_j = rows["python_onnx"]["median_joules_per_completed_correct_task"]
        rows["native_vs_cpp"] = {
            "energy_ratio": native / cpp_j,
            "energy_delta_percent": 100.0 * (1.0 - native / cpp_j),
        }
        rows["native_vs_python"] = {
            "energy_ratio": native / py_j,
            "energy_delta_percent": 100.0 * (1.0 - native / py_j),
        }
        cells.append({"cell": cell, "runners": rows})
    return {
        "schema": SCHEMA,
        "available": True,
        "evidence_class": "E1",
        "method": method,
        "boundary": boundary,
        "physical_system_energy_measured": False,
        "component_energy_measured": True,
        "measurement_quality": "diagnostic",
        "measurement_uncertainty_percent": None,
        "hardware": hardware,
        "cells": cells,
        "interpretation": (
            "CPU component energy includes host activity during each process window and is not "
            "process-attributed. Balanced runner order reduces temporal bias but does not "
            "turn hosted CI into a calibrated whole-system measurement."
        ),
        "claim_authorized": False,
        **CLAIMS,
    }


def markdown(summary: Dict[str, object]) -> str:
    if not summary.get("available"):
        return (
            "# CI component-energy comparison\n\n"
            f"Energy comparison unavailable: {summary.get('reason', 'unsupported runner')}.\n\n"
            "No joules were synthesized from latency, TDP, utilisation or unmatched coefficients.\n"
        )
    lines = [
        "# CI component-energy comparison",
        "",
        "**Evidence class E1:** hardware-reported CPU package/socket energy. "
        "This is not whole-system AC energy and is diagnostic on hosted CI.",
        "",
        "| Cell | ShortHand J/task | C++/ORT J/task | Python/ORT J/task | "
        "ShortHand vs C++ % | ShortHand vs Python % |",
        "| --- | ---: | ---: | ---: | ---: | ---: |",
    ]
    for cell in summary["cells"]:
        r = cell["runners"]
        lines.append(
            f"| {cell['cell']} | "
            f"{r['native']['median_joules_per_completed_correct_task']:.9f} | "
            f"{r['cpp_onnx']['median_joules_per_completed_correct_task']:.9f} | "
            f"{r['python_onnx']['median_joules_per_completed_correct_task']:.9f} | "
            f"{r['native_vs_cpp']['energy_delta_percent']:.2f} | "
            f"{r['native_vs_python']['energy_delta_percent']:.2f} |"
        )
    lines += [
        "",
        "Each cell uses all six permutations of the three runner orders. No successful "
        "observation is dropped. Values include process startup/model loading/warmup/report I/O.",
        "",
        "These values do **not** authorize a comparative whole-system energy or carbon claim.",
        "",
    ]
    return "\n".join(lines)


def run(args) -> int:
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    capability = evidence.probe(profile_dir=args.profiles)
    write(output / "capability.json", capability)
    if capability.get("highest_available_evidence_class") != "E1":
        result = {
            "schema": SCHEMA,
            "available": False,
            "evidence_class": None,
            "highest_available_evidence_class": capability.get("highest_available_evidence_class"),
            "reason": (
                "readable CPU package/socket energy counters unavailable; E2/E3 profiles are not "
                "silently applied without the matching required runtime features"
            ),
            "physical_system_energy_measured": False,
            "component_energy_measured": False,
            "claim_authorized": False,
            **CLAIMS,
        }
        write(output / "summary.json", result)
        (output / "ENERGY.md").write_text(markdown(result))
        return 0

    tool = cpp.resolve_executable(args.tool)
    control = cpp.resolve_executable(args.cpp)
    python = cpp.resolve_executable(args.python)
    if subprocess.check_output([control, "--version"], text=True).strip() != "1.30.0":
        raise ValueError("pinned C++ ONNX Runtime 1.30.0 required")

    fixtures = output / "inputs"
    raw: Dict[str, Dict[str, List[Dict[str, object]]]] = {}
    for batch, threads in CELLS:
        cell = f"b{batch}-t{threads}"
        fixture = fixtures / cell
        prepare_fixture(fixture, batch, threads)
        raw[cell] = {name: [] for name in RUNNERS}
        for block, order in enumerate(balanced_orders(args.seed + batch * 31 + threads)):
            directory = output / "runs" / cell / str(block)
            directory.mkdir(parents=True)
            write(directory / "order.json", order)
            commands = runner_commands(tool, control, python, fixture, directory, batch, threads)
            for name in order:
                measurement = measured_process(commands[name], directory, name)
                raw[cell][name].append(measurement)
            validate_block(directory, fixture)

    summary = summarize(raw, capability["hardware"])
    write(output / "summary.json", summary)
    (output / "ENERGY.md").write_text(markdown(summary))
    files = {str(p.relative_to(output)): campaign.sha(p)
             for p in output.rglob("*") if p.is_file()}
    write(output / "manifest.json", {
        "schema": SCHEMA,
        "success": True,
        "hashes": files,
        "claim_authorized": False,
    })
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", required=True)
    parser.add_argument("--cpp", required=True)
    parser.add_argument("--python", required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--profiles", type=pathlib.Path,
                        default=pathlib.Path(__file__).with_name("calibration_profiles"))
    parser.add_argument("--seed", type=int, default=111)
    args = parser.parse_args()
    return run(args)


if __name__ == "__main__":
    sys.exit(main())
