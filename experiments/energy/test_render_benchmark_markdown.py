#!/usr/bin/env python3
import json
import pathlib
import tempfile
import unittest

import render_benchmark_markdown as report


class BenchmarkMarkdownTest(unittest.TestCase):
    def test_renders_measurements_and_keeps_unavailable_energy_null(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            (root / "ci-energy").mkdir()
            (root / "generated-infer").mkdir()
            (root / "pr-runtime-delta").mkdir()
            (root / "run-evidence.json").write_text(json.dumps(dict(revision="abc", status="incomplete", missing=[])))
            (root / "generated-infer/report.json").write_text(json.dumps(dict(
                success=True, latency_claim_authorized=False, energy_claim_authorized=False,
                rows=[dict(cell="b1-t1", head_base_ratio=.2, head_direct_ratio=10,
                           observations=dict(head=dict(mean_ms_per_vector=.1)))])))
            (root / "pr-runtime-delta/summary.json").write_text(json.dumps(dict(
                success=True, production_claim=False, comparative_energy_claim=False,
                latency_claim_eligible=False, measured_energy_available=False,
                cells=[dict(cell="b1-t1", head_median_us_per_image=1,
                            base_median_us_per_image=2, paired_delta_percent_median=50,
                            paired_ratio_min=.4, paired_ratio_max=.6)])))
            (root / "ci-energy/summary.json").write_text(json.dumps(dict(
                available=False, hardware_measured_joules=None,
                joules_per_task=None, average_component_watts=None,
                reason="meter unavailable")))
            for name in ("cache-tests.txt", "workspace-tests.txt", "application-tests.txt",
                         "prepared-sanitizers.txt", "evidence-tests.txt"):
                (root / name).write_text("PASS " + name)
            value = report.render(root, "https://example.test/run")
            self.assertIn("# Testing and benchmark results", value)
            self.assertIn("0.2000", value)
            self.assertIn("meter unavailable", value)
            self.assertIn("Joules, joules per task and watts are therefore `null`/unavailable", value)

    def test_rejects_energy_numbers_when_meter_is_unavailable(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            for directory in ("ci-energy", "generated-infer", "pr-runtime-delta"):
                (root / directory).mkdir()
            (root / "run-evidence.json").write_text(json.dumps(dict(revision="abc", status="complete", missing=[])))
            (root / "generated-infer/report.json").write_text(json.dumps(dict(success=True, latency_claim_authorized=False, energy_claim_authorized=False, rows=[])))
            (root / "pr-runtime-delta/summary.json").write_text(json.dumps(dict(success=True, production_claim=False, comparative_energy_claim=False, latency_claim_eligible=False, measured_energy_available=False, cells=[])))
            (root / "ci-energy/summary.json").write_text(json.dumps(dict(available=False, hardware_measured_joules=1.0, average_component_watts=None)))
            for name in ("cache-tests.txt", "workspace-tests.txt", "application-tests.txt", "prepared-sanitizers.txt", "evidence-tests.txt"):
                (root / name).write_text("PASS " + name)
            with self.assertRaises(ValueError): report.render(root)


if __name__ == "__main__":
    unittest.main()
