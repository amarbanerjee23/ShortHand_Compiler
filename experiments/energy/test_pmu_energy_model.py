#!/usr/bin/env python3
"""Unit tests for the calibrated PMU-to-Joules energy attribution model."""
from __future__ import annotations

import json
import pathlib
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import pmu_energy_model as pm  # noqa: E402


class PmuEnergyModelTests(unittest.TestCase):
    def test_fit_recovers_synthetic_energy_components_and_denies_claim(self):
        # True synthetic physics: 12 W idle + 1.5e-9 J/cycle + 0.8e-9 J/instr + 5e-7 J/llc_miss
        samples = []
        for i in range(1, 13):
            sec = 0.2 * i
            cyc = 1.0e9 * (0.3 * i)
            ins = 1.5e9 * (0.25 * i + (i % 3) * 0.1)
            llc = 1.0e5 * (i % 5 + 1)
            joules = 12.0 * sec + 1.5e-9 * cyc + 0.8e-9 * ins + 5.0e-7 * llc
            samples.append({
                "seconds": sec,
                "cycles": cyc,
                "instructions": ins,
                "llc_misses": llc,
                "net_joules": joules,
            })

        profile = pm.fit_model(samples, ridge_alpha=1e-9)
        self.assertEqual(profile["evidence_class"], "calibrated_pmu_estimate")
        self.assertFalse(profile["energy_claim_authorized"])
        self.assertGreater(profile["fit_metrics"]["r2"], 0.999)
        self.assertLess(profile["fit_metrics"]["mae_joules"], 0.05)

        pred = pm.predict_joules(profile["coefficients"], samples[5])
        self.assertEqual(pred["evidence_class"], "calibrated_pmu_estimate")
        self.assertFalse(pred["energy_claim_authorized"])
        self.assertAlmostEqual(pred["estimated_joules"], samples[5]["net_joules"], delta=0.1)
        self.assertEqual(set(pred["component_attribution_joules"]), set(pm.FEATURES))

    def test_rejects_insufficient_or_negative_samples(self):
        with self.assertRaises(ValueError):
            pm.fit_model([{"seconds": 1.0, "cycles": 1.0, "instructions": 1.0, "llc_misses": 1.0, "net_joules": 1.0}])
        with self.assertRaises(ValueError):
            pm.fit_model([
                {"seconds": -1.0, "cycles": 1.0, "instructions": 1.0, "llc_misses": 1.0, "net_joules": 1.0}
                for _ in range(5)
            ])

    def test_cli_roundtrip(self):
        samples = [
            {"seconds": 0.1 * i, "cycles": 1e8 * i, "instructions": 1e8 * (i + 1), "llc_misses": 1e4 * (i % 3 + 1),
             "net_joules": 1.5 * i}
            for i in range(1, 8)
        ]
        with tempfile.TemporaryDirectory() as tmp:
            t = pathlib.Path(tmp)
            s_path = t / "samples.json"
            p_path = t / "profile.json"
            o_path = t / "obs.json"
            r_path = t / "pred.json"
            s_path.write_text(json.dumps(samples))
            o_path.write_text(json.dumps(samples[0]))
            self.assertEqual(pm.main(["fit", "--samples", str(s_path), "--output", str(p_path)]), 0)
            self.assertEqual(pm.main(["predict", "--profile", str(p_path), "--observation", str(o_path),
                                      "--output", str(r_path)]), 0)
            pred = json.loads(r_path.read_text())
            self.assertFalse(pred["energy_claim_authorized"])
            self.assertIn("profile_sha256", pred)


if __name__ == "__main__":
    unittest.main()
