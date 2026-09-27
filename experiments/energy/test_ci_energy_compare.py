#!/usr/bin/env python3
import unittest

import ci_energy_compare as compare


class CiEnergyCompareTest(unittest.TestCase):
    def test_balanced_orders_cover_all_runner_permutations(self):
        rows = compare.balanced_orders(111)
        self.assertEqual(len(rows), 6)
        self.assertEqual({tuple(x) for x in rows},
                         set(__import__('itertools').permutations(compare.RUNNERS)))

    def test_summary_uses_same_e1_boundary_for_ratios(self):
        def obs(value):
            return {
                "joules_per_completed_correct_task": value,
                "hardware_measured_joules": value * compare.COMPLETED,
                "method": "rapl_powercap_package",
                "boundary": "whole_process_cpu_package",
            }
        raw = {
            "b16-t1": {
                "native": [obs(2.0), obs(2.2), obs(1.8)],
                "cpp_onnx": [obs(2.5), obs(2.4), obs(2.6)],
                "python_onnx": [obs(4.0), obs(4.2), obs(3.8)],
            }
        }
        out = compare.summarize(raw, {"model_name": "test"})
        cell = out["cells"][0]["runners"]
        self.assertAlmostEqual(cell["native"]["median_joules_per_completed_correct_task"], 2.0)
        self.assertAlmostEqual(cell["native_vs_cpp"]["energy_ratio"], 0.8)
        self.assertAlmostEqual(cell["native_vs_cpp"]["energy_delta_percent"], 20.0)
        self.assertAlmostEqual(cell["native_vs_python"]["energy_delta_percent"], 50.0)
        self.assertEqual(out["evidence_class"], "E1")
        self.assertFalse(out["physical_system_energy_measured"])
        self.assertFalse(out["claim_authorized"])

    def test_markdown_does_not_call_e1_whole_system_energy(self):
        text = compare.markdown({
            "available": False,
            "reason": "rapl unavailable",
        })
        self.assertIn("No joules were synthesized", text)


if __name__ == "__main__":
    unittest.main()
