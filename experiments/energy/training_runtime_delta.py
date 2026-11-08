#!/usr/bin/env python3
"""Paired bounded-CNN base/candidate evidence; no inferred energy savings."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[2]
TRAINING = "Compiler_new_ws/Short_Hand/src/ai_runtime/training/TrainingQualification.cpp"
REQUIRED = {"observations.json", "report.json", "commands.json", "compiler.txt", "base.cpp", "candidate.cpp", "host.cpp", "source.diff", "training_runtime_delta.py",
            "sources/ai_runtime/training/PersistentWorkers.h", "sources/ai_runtime/training/TrainingQualification.h",
            "sources/ai_runtime/ExecutionPlan.cpp", "sources/ai_runtime/ExecutionPlan.h",
            "sources/ai_runtime/energy/EnergyMeasurement.cpp", "sources/ai_runtime/energy/EnergyMeasurement.h",
            "sources/module/Sha256.cpp", "sources/module/Sha256.h", "base", "base-counts", "candidate", "candidate-counts"}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate(rows):
    assert len(rows) == 44, "missing training observations"
    expected = set()
    for threads in (1, 2):
        for mode in ("base", "candidate"):
            expected |= {(threads, mode, "warmup", i) for i in range(2)}
            expected |= {(threads, mode, "latency", i) for i in range(6)}
            expected |= {(threads, mode, kind, 0) for kind in ("allocations", "injected", "seed-check")}
    seen = set()
    for row in rows:
        key = row["threads"], row["mode"], row["kind"], row["iteration"]
        assert key in expected and key not in seen, "duplicate/unexpected observation"
        seen.add(key)
        assert row["joules"] is None and row["samples"] == 1024 and row["steps"] == 128
        assert all(math.isfinite(row[k]) and row[k] > 0 for k in ("seconds", "final_loss", "initial_loss"))
        assert 0.95 <= row["accuracy"] <= 1 and row["final_loss"] < row["initial_loss"]
        assert re.fullmatch(r"[0-9a-f]{64}", row["parameters_sha256"])
        counted = row["kind"] in ("allocations", "injected")
        for key in ("allocations", "allocated_bytes"):
            assert (type(row[key]) is int and row[key] > 0) if counted else row[key] is None
        assert row["seed"] == (7 if row["kind"] == "seed-check" else 42)
    assert seen == expected
    for seed in (42, 7):
        selected = [r for r in rows if r["seed"] == seed]
        for key in ("parameters_sha256", "accuracy", "final_loss", "initial_loss"):
            assert len({r[key] for r in selected}) == 1, f"base/candidate/threads differ: {key}"
    for threads in (1, 2):
        for mode in ("base", "candidate"):
            counts = {r["kind"]: r for r in rows if r["threads"] == threads and r["mode"] == mode}
            assert counts["injected"]["allocations"] == counts["allocations"]["allocations"] + 1
            assert counts["injected"]["allocated_bytes"] == counts["allocations"]["allocated_bytes"] + 64


def report(rows):
    validate(rows)
    result = {}
    for threads in (1, 2):
        paired = {(r["mode"], r["iteration"]): r["seconds"] for r in rows if r["threads"] == threads and r["kind"] == "latency"}
        ratios = [paired["candidate", i] / paired["base", i] for i in range(6)]
        counts = {r["mode"]: r["allocations"] for r in rows if r["threads"] == threads and r["kind"] == "allocations"}
        result[str(threads)] = {"paired_time_ratio_median": statistics.median(ratios), "paired_time_ratios": ratios,
                                "allocation_counts": counts}
    return {"schema": "shorthand.training_delta.v1", "success": True, "boundary": "training_call_including_initialization_validation_and_worker_teardown",
            "workload": "fixed_synthetic_reference_cnn", "threads": result, "joules": None,
            "latency_claim_authorized": False, "energy_claim_authorized": False}


def replay(folder, expected):
    assert digest(folder / "manifest.json") == expected, "manifest digest mismatch"
    manifest = json.loads((folder / "manifest.json").read_text())
    assert manifest["schema"] == "shorthand.training_delta.bundle.v1"
    assert set(manifest) == {"schema", "base", "head", "files"}
    assert all(re.fullmatch(r"[0-9a-f]{40}", manifest[key]) for key in ("base", "head"))
    assert REQUIRED <= manifest["files"].keys(), "missing source/binary evidence"
    for name, value in manifest["files"].items():
        path = Path(name)
        assert not path.is_absolute() and ".." not in path.parts
        assert (folder / path).resolve().is_relative_to(folder.resolve()), "escaped evidence path"
        assert not (folder / path).is_symlink(), "symlink evidence"
        assert re.fullmatch(r"[0-9a-f]{64}", value)
        assert digest(folder / path) == value, f"file digest mismatch: {name}"
    computed = report(json.loads((folder / "observations.json").read_text()))
    assert computed == json.loads((folder / "report.json").read_text()), "report does not replay"
    return computed


def capture(base, folder, cxx):
    assert re.fullmatch(r"[0-9a-f]{40}", base), "immutable base SHA required"
    folder.mkdir(parents=True, exist_ok=True)
    assert not any(folder.iterdir()), "fresh output directory required"
    commands = []

    def run(args):
        commands.append(list(map(str, args)))
        return subprocess.check_output(list(map(str, args)), cwd=ROOT, text=True, stderr=subprocess.STDOUT, timeout=180)

    (folder / "base.cpp").write_text(run(["git", "show", f"{base}:{TRAINING}"]))
    (folder / "candidate.cpp").write_bytes((ROOT / TRAINING).read_bytes())
    (folder / "host.cpp").write_bytes((ROOT / "experiments/energy/training_delta_host.cpp").read_bytes())
    (folder / "training_runtime_delta.py").write_bytes(Path(__file__).read_bytes())
    # This is a training-source ablation, with identical host and common support
    # code on both sides. Require the public trainer contract to match the base.
    header = "Compiler_new_ws/Short_Hand/src/ai_runtime/training/TrainingQualification.h"
    assert run(["git", "show", f"{base}:{header}"]) == (ROOT / header).read_text(), "training ABI/configuration changed"
    (folder / "source.diff").write_text(run(["git", "diff", base, "--", "Compiler_new_ws/Short_Hand/src", "experiments/energy/training_delta_host.cpp"]))
    (folder / "compiler.txt").write_text(run([cxx, "--version"]))
    src = ROOT / "Compiler_new_ws/Short_Hand/src"
    deps = [src / p for p in ("ai_runtime/ExecutionPlan.cpp", "ai_runtime/energy/EnergyMeasurement.cpp", "module/Sha256.cpp")]
    for mode in ("base", "candidate"):
        for counted in (False, True):
            binary = folder / (mode + ("-counts" if counted else ""))
            run([cxx, "-std=c++17", "-O2", "-ffp-contract=off", "-fno-fast-math", "-pthread", "-I", src,
                 "-I", src / "ai_runtime/training", *(["-DSHORTHAND_COUNT_ALLOCATIONS"] if counted else []),
                 folder / "host.cpp", folder / (mode + ".cpp"), *deps, "-o", binary])
    rows = []
    for threads in (1, 2):
        for kind, count in (("warmup", 2), ("latency", 6), ("allocations", 1), ("injected", 1), ("seed-check", 1)):
            for iteration in range(count):
                for mode in (("base", "candidate") if iteration % 2 == 0 else ("candidate", "base")):
                    counted = kind in ("allocations", "injected")
                    seed = 7 if kind == "seed-check" else 42
                    output = run([folder / (mode + ("-counts" if counted else "")), threads, seed, "inject" if kind == "injected" else "plain"])
                    row = json.loads(output)
                    row.update(threads=threads, mode=mode, kind=kind, iteration=iteration, seed=seed)
                    rows.append(row)
    result = report(rows)
    for name, data in (("observations.json", rows), ("report.json", result), ("commands.json", commands)):
        (folder / name).write_text(json.dumps(data, indent=2, sort_keys=True) + "\n")
    # Retain all source dependencies and private headers used in compilation.
    for path in [*deps, src / "ai_runtime/ExecutionPlan.h", src / "ai_runtime/energy/EnergyMeasurement.h",
                 src / "module/Sha256.h", src / "ai_runtime/training/TrainingQualification.h", src / "ai_runtime/training/PersistentWorkers.h"]:
        target = folder / "sources" / path.relative_to(src)
        target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(path.read_bytes())
    manifest = {"schema": "shorthand.training_delta.bundle.v1", "base": base, "head": run(["git", "rev-parse", "HEAD"]).strip(),
                "files": {str(p.relative_to(folder)): digest(p) for p in sorted(folder.rglob("*")) if p.is_file()}}
    (folder / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    expected = digest(folder / "manifest.json")
    (folder / "manifest.sha256").write_text(expected + "\n")
    replay(folder, expected)
    print(json.dumps({"manifest_sha256": expected, "report": result}, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--replay")
    args = parser.parse_args()
    if args.replay:
        print(json.dumps(replay(args.output.resolve(), args.replay), indent=2))
    else:
        capture(args.base or "", args.output.resolve(), args.cxx)
