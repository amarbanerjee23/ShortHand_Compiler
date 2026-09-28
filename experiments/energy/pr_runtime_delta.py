#!/usr/bin/env python3
"""Same-runner PR head/base resident runtime delta.

This captures only software/runtime latency evidence. It deliberately does not
infer energy from latency and does not authorize a comparative performance,
energy, carbon, production, or certification claim.

For every declared cell the head and base binaries execute in balanced AB/BA
order on the same runner against one frozen fixture. All completed blocks are
retained and numerical/classification parity is mandatory.
"""
from __future__ import annotations

import argparse
import json
import math
import os
import pathlib
import platform
import random
import statistics
import subprocess
import sys
import time
from typing import Dict, List

import campaign
import runtime_state_of_practice as cpp

ROOT = pathlib.Path(__file__).resolve().parents[2]
SCHEMA = "shorthand.energy.pr_runtime_delta.v1"
RUNNERS = ("head", "base")
CELLS = ((1, 1), (16, 1), (32, 1), (16, 2), (16, 4))
CLAIMS = dict(
    production_claim=False,
    comparative_energy_claim=False,
    official_certification_granted=False,
    lowest_carbon_language_claim=False,
    latency_claim_eligible=False,
    measured_energy_available=False,
)


def balanced_orders(seed: int, blocks: int = 6) -> List[List[str]]:
    if type(blocks) is not int or blocks < 4 or blocks > 20 or blocks % 2:
        raise ValueError("even block count from 4 to 20 required")
    result = [["head", "base"], ["base", "head"]] * (blocks // 2)
    random.Random(seed).shuffle(result)
    return result


def write(path: pathlib.Path, value) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n")


def load(path: pathlib.Path):
    return campaign.load(path)


def run_process(argv, stdout_path: pathlib.Path, stderr_path: pathlib.Path) -> float:
    start = time.perf_counter()
    with stdout_path.open("wb") as stdout, stderr_path.open("wb") as stderr:
        proc = subprocess.run(
            list(map(str, argv)),
            stdin=subprocess.DEVNULL,
            stdout=stdout,
            stderr=stderr,
            env=dict(os.environ, **campaign.ENV),
            timeout=300,
            check=False,
        )
    elapsed = (time.perf_counter() - start) * 1000
    if proc.returncode:
        raise ValueError(
            f"runner failed with {proc.returncode}: "
            + stderr_path.read_text(errors="replace")[-4000:]
        )
    return elapsed


def resident_us(report: Dict[str, object]) -> float:
    trials = report.get("trials")
    if not isinstance(trials, list) or len(trials) != 3:
        raise ValueError("three complete trials required")
    values = []
    for trial in trials:
        if trial.get("success") is not True:
            raise ValueError("unsuccessful trial")
        completed = trial.get("completed")
        elapsed = trial.get("elapsed_ms")
        if type(completed) is not int or completed <= 0:
            raise ValueError("invalid completed functional units")
        if type(elapsed) not in (int, float) or not math.isfinite(elapsed) or elapsed <= 0:
            raise ValueError("invalid resident elapsed time")
        values.append(float(elapsed) * 1000.0 / completed)
    return statistics.median(values)


def validate_equivalence(head, base, q) -> None:
    required_equal = (
        "configuration_sha256",
        "qualification_sha256",
        "model_sha256",
        "dataset_sha256",
        "dataset_id",
        "dataset_split",
        "precision",
        "backend",
        "backend_version",
        "threads",
        "batch_size",
        "functional_unit",
        "rows",
        "minimum_accuracy",
    )
    if head.get("success") is not True or base.get("success") is not True:
        raise ValueError("head/base execution unsuccessful")
    for key in required_equal:
        if head.get(key) != base.get(key):
            raise ValueError("head/base workload mismatch: " + key)
    if head.get("predictions") != base.get("predictions") or head.get("top_k") != base.get("top_k"):
        raise ValueError("head/base classification mismatch")
    hs, bs = head.get("scores"), base.get("scores")
    if not isinstance(hs, list) or not isinstance(bs, list) or len(hs) != len(bs) or not hs:
        raise ValueError("head/base score shape mismatch")
    atol = q.get("absolute_tolerance", 1e-5)
    rtol = q.get("relative_tolerance", 1e-4)
    for a, b in zip(hs, bs):
        if type(a) not in (int, float) or type(b) not in (int, float):
            raise ValueError("non-numeric score")
        if not math.isfinite(a) or not math.isfinite(b) or abs(a - b) > atol + rtol * abs(b):
            raise ValueError("head/base numerical mismatch")
    for report in (head, base):
        if report.get("accuracy", 0) < report.get("minimum_accuracy", 1):
            raise ValueError("quality guardrail regression")
        resident_us(report)


def summarize_cell(observations: List[Dict[str, float]]) -> Dict[str, object]:
    if not observations:
        raise ValueError("missing paired observations")
    head = [row["head_us_per_image"] for row in observations]
    base = [row["base_us_per_image"] for row in observations]
    ratios = [row["ratio"] for row in observations]
    return {
        "blocks": len(observations),
        "head_median_us_per_image": statistics.median(head),
        "base_median_us_per_image": statistics.median(base),
        "paired_ratio_median": statistics.median(ratios),
        "paired_delta_percent_median": statistics.median(100.0 * (1.0 - r) for r in ratios),
        "paired_ratio_min": min(ratios),
        "paired_ratio_max": max(ratios),
        "paired_ratios": ratios,
    }


def markdown(summary) -> str:
    lines = [
        "# Same-runner PR runtime delta",
        "",
        "Balanced PR head/base observations on one hosted runner. Descriptive evidence only;",
        "no hosted timing threshold, speedup claim, energy inference, or carbon claim is authorized.",
        "",
        f"Head: `{summary['head_sha']}`  ",
        f"Base: `{summary['base_sha']}`",
        "",
        "| Cell | Head µs/image | Base µs/image | Head/Base | Median delta | Ratio range |",
        "| --- | ---: | ---: | ---: | ---: | --- |",
    ]
    for row in summary["cells"]:
        lines.append(
            f"| {row['cell']} | {row['head_median_us_per_image']:.6f} | "
            f"{row['base_median_us_per_image']:.6f} | {row['paired_ratio_median']:.4f} | "
            f"{row['paired_delta_percent_median']:.2f}% | "
            f"{row['paired_ratio_min']:.4f}–{row['paired_ratio_max']:.4f} |"
        )
    lines += [
        "",
        "Each block runs both binaries against the same frozen fixture; AB and BA occur equally often.",
        "Predictions, top-k, score tolerances, accuracy, model, dataset, precision, backend and functional units must match.",
        "Latency is not converted to joules. Energy evidence remains in the independent E0/E1/E2/E3 pipeline.",
        "",
    ]
    return "\n".join(lines)


def run(args) -> int:
    if platform.system() != "Linux" or sys.version_info[:2] != (3, 12):
        raise ValueError("same-runner PR delta requires Linux CPython 3.12")
    if args.head_sha == args.base_sha:
        raise ValueError("head and base revisions must differ")
    head_tool = cpp.resolve_executable(args.head_tool)
    base_tool = cpp.resolve_executable(args.base_tool)
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    plan = {
        "schema": SCHEMA,
        "head_sha": args.head_sha,
        "base_sha": args.base_sha,
        "head_binary_sha256": campaign.sha(head_tool),
        "base_binary_sha256": campaign.sha(base_tool),
        "cells": [f"b{b}-t{t}" for b, t in CELLS],
        "blocks": args.blocks,
        "seed": args.seed,
        "orders": {},
        **CLAIMS,
    }
    observations = {}
    try:
        for batch, threads in CELLS:
            cell = f"b{batch}-t{threads}"
            fixture = out / "inputs" / cell
            app_path = campaign.create(fixture, batch=batch, threads=threads)
            app = campaign.load(app_path)
            dataset = pathlib.Path(app["dataset_path"])
            campaign.snapshot(dataset, fixture / "dataset.csv")
            app["dataset_path"] = str(fixture / "dataset.csv")
            campaign.write(app_path, app)
            q = campaign.load(fixture / "qualification.json")
            plan["orders"][cell] = balanced_orders(args.seed + batch * 31 + threads, args.blocks)
            cell_rows = []
            for index, order in enumerate(plan["orders"][cell]):
                directory = out / "runs" / cell / str(index)
                directory.mkdir(parents=True)
                write(directory / "order.json", order)
                reports = {}
                process_ms = {}
                for name in order:
                    report_path = directory / f"{name}-report.json"
                    tool = head_tool if name == "head" else base_tool
                    process_ms[name] = run_process(
                        [tool, "application", app_path, report_path],
                        directory / f"{name}.stdout",
                        directory / f"{name}.stderr",
                    )
                    report = cpp.validate_native_report(report_path)
                    reports[name] = report
                validate_equivalence(reports["head"], reports["base"], q)
                head_us = resident_us(reports["head"])
                base_us = resident_us(reports["base"])
                ratio = head_us / base_us
                if not math.isfinite(ratio) or ratio <= 0:
                    raise ValueError("invalid paired runtime ratio")
                row = {
                    "block": index,
                    "order": order,
                    "head_us_per_image": head_us,
                    "base_us_per_image": base_us,
                    "ratio": ratio,
                    "head_process_ms": process_ms["head"],
                    "base_process_ms": process_ms["base"],
                }
                write(directory / "paired-metric.json", row)
                cell_rows.append(row)
            observations[cell] = cell_rows

        cells = []
        for batch, threads in CELLS:
            cell = f"b{batch}-t{threads}"
            cells.append({"cell": cell, **summarize_cell(observations[cell])})
        summary = {
            "schema": SCHEMA,
            "success": True,
            "head_sha": args.head_sha,
            "base_sha": args.base_sha,
            "cells": cells,
            **CLAIMS,
        }
        write(out / "plan.json", plan)
        write(out / "summary.json", summary)
        (out / "PR_DELTA.md").write_text(markdown(summary))
        hashes = {
            str(path.relative_to(out)): campaign.sha(path)
            for path in out.rglob("*")
            if path.is_file()
        }
        write(out / "manifest.json", {
            "schema": SCHEMA,
            "success": True,
            "head_sha": args.head_sha,
            "base_sha": args.base_sha,
            "hashes": hashes,
            **CLAIMS,
        })
        print(markdown(summary))
        return 0
    except Exception as exc:
        write(out / "failure.json", {
            "schema": SCHEMA,
            "success": False,
            "error": str(exc),
            "head_sha": args.head_sha,
            "base_sha": args.base_sha,
            **CLAIMS,
        })
        raise


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--head-tool", required=True)
    parser.add_argument("--base-tool", required=True)
    parser.add_argument("--head-sha", required=True)
    parser.add_argument("--base-sha", required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--blocks", type=int, default=6)
    parser.add_argument("--seed", type=int, default=112)
    args = parser.parse_args()
    balanced_orders(args.seed, args.blocks)
    return run(args)


if __name__ == "__main__":
    sys.exit(main())
