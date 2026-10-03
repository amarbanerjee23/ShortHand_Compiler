#!/usr/bin/env python3
"""Render the retained per-run test and benchmark results as Markdown.

The report is generated inside the evidence workflow before finalization, so the
same artifact contains the measurements, test markers, and the integrity index
for that exact revision. It never turns unavailable energy into zero.
"""
from __future__ import annotations

import argparse
import json
import os
import pathlib
from typing import Any


def read_json(path: pathlib.Path) -> dict[str, Any]:
    value = json.loads(path.read_text())
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path}")
    return value


def read_text(path: pathlib.Path) -> str:
    if not path.is_file() or not path.stat().st_size:
        raise ValueError(f"missing or empty test log: {path}")
    return path.read_text(errors="replace")


def fmt(value: Any, digits: int = 3) -> str:
    if value is None:
        return "unavailable"
    if not isinstance(value, (int, float)) or isinstance(value, bool):
        return "invalid"
    return f"{value:.{digits}f}"


def render(profile: pathlib.Path, run_url: str = "") -> str:
    evidence = read_json(profile / "run-evidence.json")
    generated = read_json(profile / "generated-infer/report.json")
    resident = read_json(profile / "pr-runtime-delta/summary.json")
    energy = read_json(profile / "ci-energy/summary.json")
    required = ("cache-tests.txt", "workspace-tests.txt", "application-tests.txt",
                "prepared-sanitizers.txt", "evidence-tests.txt")
    logs = {name: read_text(profile / name) for name in required}
    if generated.get("success") is not True or resident.get("success") is not True:
        raise ValueError("successful benchmark reports are required")
    if any(generated.get(key) is not False for key in ("latency_claim_authorized", "energy_claim_authorized")):
        raise ValueError("generated report must keep reduction claims disabled")
    if any(resident.get(key) is not False for key in
           ("production_claim", "comparative_energy_claim", "latency_claim_eligible",
            "measured_energy_available")):
        raise ValueError("resident report must keep claims disabled")
    if energy.get("available") is False and any(energy.get(k) is not None for k in
                                                 ("hardware_measured_joules", "joules_per_task",
                                                  "average_component_watts")):
        raise ValueError("unavailable energy cannot contain numeric measurements")

    lines = [
        "# Testing and benchmark results",
        "",
        "This report is generated from the exact retained artifacts for one CI run. "
        "It is descriptive evidence; it does not authorize a language-wide latency, power, or energy claim.",
        "",
        f"- Revision: `{evidence.get('revision', 'unknown')}`",
        f"- Comparison base: `{resident.get('base_sha', 'unknown')}` (the exact selected Git revision; it may already include session reuse)",
        f"- Collection status: **{evidence.get('status', 'unknown')}**",
        f"- Missing evidence at render time: `{', '.join(evidence.get('missing', [])) or 'none'}`",
    ]
    if run_url:
        lines.append(f"- CI run: [{run_url}]({run_url})")
    lines += [
        "",
        "## Test coverage",
        "",
        "| Area | Retained log | Required result |",
        "|---|---|---|",
        "| Real ONNX cache and lifecycle | `cache-tests.txt` | cache reuse, invalidation, boundaries, external-weight recovery, concurrency and soak markers |",
        "| Resident workspace | `workspace-tests.txt` | correctness, rollback, padding and 2,000-call alternating full/tail soak |",
        "| Real-digit application | `application-tests.txt` | held-out application correctness and quality gate |",
        "| Sanitizers | `prepared-sanitizers.txt` | instrumented prepared cache/workspace ASan/LSan/UBSan pass |",
        "| Evidence integrity | `evidence-tests.txt` | malformed samples, tampering, unsafe paths, meter failures and claim gating |",
        "",
        "The cache test includes 2,000 sequential calls, 4,000 concurrent calls while 64 resets/re-registrations run, 65,536 and 65,537-float bridge requests, 16 MiB and 16 MiB+1 model snapshots, external-weight deletion/recovery, invalid argument matrices, output canaries and aliasing checks.",
        "",
        "## Compiled generated workload",
        "",
        "Synthetic zero-input FP32 MatMul+Add probe; one thread; 16 timed blocks per runner/cell after warmup. It verifies output numerically against the pinned ORT oracle and does not measure digit accuracy.",
        "",
        "| Cell | Head mean ms/vector | Head/base | Head/direct prepared C++ ORT |",
        "|---|---:|---:|---:|",
    ]
    for row in generated["rows"]:
        lines.append(f"| {row['cell']} | {fmt(row['observations']['head']['mean_ms_per_vector'], 6)} | "
                     f"{fmt(row.get('head_base_ratio'), 4)} | {fmt(row.get('head_direct_ratio'), 3)} |")
    lines += [
        "",
        "A head/base ratio below 1 means the head runtime was faster than the selected base revision for that probe. The base is not necessarily a repeated-session implementation. A head/direct ratio above 1 means it remained slower than the direct prepared C++/ORT control.",
        "",
        "## Resident real-digit workload",
        "",
        "All 1,797 held-out Optdigits images were checked for prediction, top-k, numerical and quality parity. Each cell contains six balanced paired blocks; the reduction is the median of paired reductions.",
        "",
        "| Cell | Head median µs/image | Base median µs/image | Paired median reduction | Paired ratio range |",
        "|---|---:|---:|---:|---|",
    ]
    for row in resident["cells"]:
        lines.append(f"| {row['cell']} | {fmt(row['head_median_us_per_image'], 6)} | "
                     f"{fmt(row['base_median_us_per_image'], 6)} | {fmt(row['paired_delta_percent_median'], 2)}% | "
                     f"{fmt(row['paired_ratio_min'], 4)}–{fmt(row['paired_ratio_max'], 4)} |")
    lines += [
        "",
        "Individual paired blocks can regress. Hosted-runner timings are descriptive and do not establish a confidence-bound performance claim.",
        "",
        "## Energy and power",
        "",
    ]
    if energy.get("available") is False:
        lines += [
            "Energy measurement was unavailable for this run: " + str(energy.get("reason", "no reason recorded")) + ".",
            "Joules, joules per task and watts are therefore `null`/unavailable. Latency is not converted into energy, and no energy or power saving is reported.",
        ]
    else:
        lines += [
            f"Measured energy class: `{energy.get('evidence_class', 'unknown')}`.",
            f"Hardware joules: `{energy.get('hardware_measured_joules', 'unavailable')}`; joules/task: `{energy.get('joules_per_task', 'unavailable')}`; average component watts: `{energy.get('average_component_watts', 'unavailable')}`.",
        ]
    lines += [
        "",
        "## Reproduction and evidence boundaries",
        "",
        "The workflow retains source, model, dataset, compiler, SDK, binary and command hashes, raw stdout/stderr, reports, and an evidence index. Replay checks the manifest digest, every indexed file hash, safe relative paths, observation cardinality, numeric types, and the no-claim contract.",
        "",
        "The compiled lane is a runtime-overhead experiment. The resident lane is the real-digit application comparison. These measurements do not prove lower average power, lower joules/task, superiority to optimized C++/ORT, or a language-wide result. Archive the retained CI bundle before its 90-day retention expires if the result must remain citable.",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile-dir", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--run-url", default="")
    args = parser.parse_args()
    run_url = args.run_url or os.environ.get("GITHUB_RUN_URL", "")
    args.output.write_text(render(args.profile_dir, run_url))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
