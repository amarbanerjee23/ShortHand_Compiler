#!/usr/bin/env python3
"""PR98 strict manifest and claims-safety regression tests."""
import copy
import importlib.util
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1]).resolve()
module_path = root / "scripts/validate_ai_benchmark_suite.py"
spec = importlib.util.spec_from_file_location("benchmark_validator", module_path)
validator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)
manifest = json.loads((root / "tests/ai_benchmark/benchmark_suite_v1.json").read_text())
checks = 0


def good(value):
    global checks
    validator.validate(value, root)
    checks += 1


def bad(mutator, reason):
    global checks
    value = copy.deepcopy(manifest)
    mutator(value)
    try:
        validator.validate(value, root)
    except ValueError as exc:
        assert reason in str(exc), (reason, str(exc))
    else:
        raise AssertionError("invalid benchmark manifest accepted")
    checks += 1


good(manifest)
bad(lambda x: x["families"].pop(), "benchmark_family_count")
bad(lambda x: x["families"].__setitem__(1, copy.deepcopy(x["families"][0])), "invalid_or_duplicate")
bad(lambda x: x.__setitem__("production_scope", "gpu"), "invalid_benchmark_production_scope")
bad(lambda x: x.__setitem__("comparative_energy_claim", True), "unsupported_suite_claim")
bad(lambda x: x.__setitem__("official_certification_granted", True), "unsupported_suite_claim")
bad(lambda x: x.__setitem__("lowest_carbon_language_claim", True), "unsupported_suite_claim")
bad(lambda x: x["families"][1].__setitem__("standard_dataset_quality", True), "standard_quality_requires_real_held_out_dataset")
bad(lambda x: x["families"][2].__setitem__("baseline_status", "qualified_execution_comparison"), "qualified_baseline_requires_evidence")
bad(lambda x: x["families"][0].__setitem__("baseline_status", "equivalent_baseline_pending"), "pending_baseline_must_not_attach_qualified_evidence")
bad(lambda x: x["families"][0]["execution_evidence"].__setitem__(0, "../outside"), "unsafe_evidence_path")
bad(lambda x: x["families"][4].__setitem__("energy_status", "measured"), "unqualified_energy_status")
bad(lambda x: x.__setitem__("full_standard_dataset_coverage", True), "unsupported_suite_claim")
print(f"PASS {checks} PR98 benchmark family contract and claims-safety cases")
