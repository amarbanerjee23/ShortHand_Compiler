#!/usr/bin/env python3
"""Validate PR98 family evidence without converting fixtures into production claims."""
import json
import pathlib
import sys

FAMILIES = {
    "classification", "detection", "retrieval", "training", "quantized_inference",
    "batched_inference", "concurrent_serving",
}
ENTRY_KEYS = {
    "family", "evidence_class", "quality_scope", "execution_evidence", "baseline_evidence",
    "baseline_status", "energy_status", "standard_dataset_quality", "production_claim",
}
TOP_KEYS = {
    "schema", "suite_id", "production_scope", "families", "full_standard_dataset_coverage",
    "full_equivalent_baseline_coverage", "calibrated_energy_coverage", "comparative_energy_claim",
    "official_certification_granted", "lowest_carbon_language_claim",
}
EVIDENCE_CLASSES = {
    "real_held_out_dataset", "controlled_model_and_execution_fixture",
    "deterministic_execution_fixture", "native_training_correctness_fixture",
}
BASELINE = {"qualified_execution_comparison", "equivalent_baseline_pending"}


def require(value, reason):
    if not value:
        raise ValueError(reason)


def safe_repo_path(root, raw):
    require(isinstance(raw, str) and raw and len(raw) <= 4096, "invalid_evidence_path")
    path = pathlib.PurePosixPath(raw)
    require(not path.is_absolute() and ".." not in path.parts, "unsafe_evidence_path")
    resolved = (root / path).resolve()
    require(root.resolve() in resolved.parents, "evidence_path_escape")
    require(resolved.is_file() and not resolved.is_symlink(), "missing_evidence_path:" + raw)


def validate(data, root):
    require(isinstance(data, dict) and set(data) == TOP_KEYS, "invalid_benchmark_suite_keys")
    require(data["schema"] == "shorthand.ai.benchmark_suite.v1", "invalid_benchmark_suite_schema")
    require(isinstance(data["suite_id"], str) and 0 < len(data["suite_id"]) <= 256, "invalid_suite_id")
    require(data["production_scope"] == "linux-x64-cpu-v1", "invalid_benchmark_production_scope")
    entries = data["families"]
    require(isinstance(entries, list) and len(entries) == len(FAMILIES), "benchmark_family_count")
    seen = set()
    real_quality = set()
    qualified_baselines = set()
    for entry in entries:
        require(isinstance(entry, dict) and set(entry) == ENTRY_KEYS, "invalid_benchmark_family_keys")
        family = entry["family"]
        require(family in FAMILIES and family not in seen, "invalid_or_duplicate_benchmark_family")
        seen.add(family)
        require(entry["evidence_class"] in EVIDENCE_CLASSES, "invalid_benchmark_evidence_class")
        require(isinstance(entry["quality_scope"], str) and 0 < len(entry["quality_scope"]) <= 1024, "invalid_quality_scope")
        execution = entry["execution_evidence"]
        baseline = entry["baseline_evidence"]
        require(isinstance(execution, list) and 1 <= len(execution) <= 8, "missing_execution_evidence")
        require(isinstance(baseline, list) and len(baseline) <= 8, "invalid_baseline_evidence")
        for path in execution + baseline:
            safe_repo_path(root, path)
        require(entry["baseline_status"] in BASELINE, "invalid_baseline_status")
        if entry["baseline_status"] == "qualified_execution_comparison":
            require(bool(baseline), "qualified_baseline_requires_evidence")
            qualified_baselines.add(family)
        else:
            require(not baseline, "pending_baseline_must_not_attach_qualified_evidence")
        require(entry["energy_status"] == "calibrated_physical_measurement_pending", "unqualified_energy_status")
        require(type(entry["standard_dataset_quality"]) is bool, "invalid_standard_dataset_quality")
        require(entry["production_claim"] is False, "benchmark_family_production_claim_forbidden")
        if entry["standard_dataset_quality"]:
            require(entry["evidence_class"] == "real_held_out_dataset", "standard_quality_requires_real_held_out_dataset")
            real_quality.add(family)
    require(seen == FAMILIES, "benchmark_family_coverage_incomplete")
    require({"classification", "batched_inference", "concurrent_serving"}.issubset(real_quality), "held_out_application_quality_missing")
    require({"classification", "batched_inference", "concurrent_serving"}.issubset(qualified_baselines), "application_baseline_evidence_missing")
    require("detection" not in real_quality and "retrieval" not in real_quality and "training" not in real_quality,
            "fixture_family_misrepresented_as_standard_dataset_quality")
    for field in ("full_standard_dataset_coverage", "full_equivalent_baseline_coverage", "calibrated_energy_coverage",
                  "comparative_energy_claim", "official_certification_granted", "lowest_carbon_language_claim"):
        require(data[field] is False, "unsupported_suite_claim:" + field)
    return True


def load(path, root):
    require(path.is_file() and not path.is_symlink() and path.stat().st_size <= 1024 * 1024, "unsafe_benchmark_manifest")
    data = json.loads(path.read_text())
    validate(data, root)
    return data


if __name__ == "__main__":
    if len(sys.argv) not in (2, 3):
        raise SystemExit("usage: validate_ai_benchmark_suite.py MANIFEST [REPO_ROOT]")
    manifest = pathlib.Path(sys.argv[1]).resolve()
    root = pathlib.Path(sys.argv[2]).resolve() if len(sys.argv) == 3 else pathlib.Path(__file__).resolve().parents[1]
    try:
        data = load(manifest, root)
    except (ValueError, json.JSONDecodeError, OSError) as exc:
        print("benchmark suite validation failed: " + str(exc), file=sys.stderr)
        raise SystemExit(2)
    print(f"PASS benchmark suite {data['suite_id']} families={len(data['families'])} claims=bounded")
