#!/usr/bin/env python3
import json
import pathlib
import tempfile
import unittest

import ci_energy_evidence as energy


def profile(evidence="E2"):
    return {
        "schema": energy.PROFILE_SCHEMA,
        "profile_id": "unit-test-profile",
        "evidence_class": evidence,
        "validated_calibration": evidence == "E2",
        "hardware_match": {"vendor_id": "UnitVendor", "cpu_family": "1", "model": "2", "machine": "x86_64"},
        "coefficients": {
            "instructions": 1e-9,
            "cache_misses": 2e-7,
            "elapsed_seconds": 0.5,
            "intercept_joules": 0.1,
        },
        "uncertainty_percent": 5.0,
        "source": "synthetic unit-test coefficients",
        "calibration_provenance": "unit-test-only; not production evidence",
    }


class EnergyEvidenceTest(unittest.TestCase):
    def setUp(self):
        self.hardware = {
            "vendor_id": "UnitVendor",
            "cpu_family": "1",
            "model": "2",
            "machine": "x86_64",
            "model_name": "Synthetic CPU",
        }

    def test_e2_estimate_is_explicitly_not_physical(self):
        p = profile("E2")
        counters = {
            "completed_correct_tasks": 100,
            "instructions": 1_000_000,
            "cache_misses": 10_000,
            "elapsed_seconds": 2.0,
        }
        out = energy.estimate(counters, p, self.hardware, "abc")
        expected = 0.1 + 1_000_000e-9 + 10_000 * 2e-7 + 2 * 0.5
        self.assertAlmostEqual(out["calibrated_joules_estimate"], expected)
        self.assertAlmostEqual(out["joules_per_completed_correct_task"], expected / 100)
        self.assertEqual(out["evidence_class"], "E2")
        self.assertFalse(out["physical_system_energy_measured"])
        self.assertFalse(out["component_energy_measured"])
        self.assertFalse(out["claim_authorized"])

    def test_e3_keeps_analytical_field_separate(self):
        p = profile("E3")
        p["validated_calibration"] = False
        out = energy.estimate({
            "completed_correct_tasks": 10,
            "instructions": 100,
            "cache_misses": 20,
            "elapsed_seconds": 1,
        }, p, self.hardware)
        self.assertEqual(out["evidence_class"], "E3")
        self.assertIsNone(out["calibrated_joules_estimate"])
        self.assertIsNotNone(out["analytical_joules_estimate"])

    def test_unvalidated_e2_rejected(self):
        p = profile("E2")
        p["validated_calibration"] = False
        with self.assertRaises(ValueError):
            energy.validate_profile(p)

    def test_mismatched_hardware_rejected(self):
        p = profile("E2")
        bad = dict(self.hardware, model="99")
        with self.assertRaises(ValueError):
            energy.estimate({
                "completed": 1,
                "instructions": 1,
                "cache_misses": 1,
                "elapsed_seconds": 1,
            }, p, bad)

    def test_missing_required_counter_rejected(self):
        with self.assertRaises(ValueError):
            energy.estimate({
                "completed": 1,
                "instructions": 1,
                "elapsed_seconds": 1,
            }, profile("E2"), self.hardware)

    def test_compare_requires_same_method_and_profile(self):
        p = profile("E2")
        a = energy.estimate({
            "completed": 1, "instructions": 10, "cache_misses": 10, "elapsed_seconds": 1
        }, p, self.hardware, "sha")
        b = energy.estimate({
            "completed": 1, "instructions": 20, "cache_misses": 20, "elapsed_seconds": 1
        }, p, self.hardware, "sha")
        result = energy.compare(a, b)
        self.assertEqual(result["evidence_class"], "E2")
        self.assertGreater(result["short_hand_to_baseline_energy_ratio"], 0)
        altered = dict(b, calibration_profile_sha256="other")
        with self.assertRaises(ValueError):
            energy.compare(a, altered)

    def test_fake_powercap_discovery_and_wrap_sampling(self):
        with tempfile.TemporaryDirectory() as d:
            root = pathlib.Path(d)
            package = root / "intel-rapl:0"
            package.mkdir()
            (package / "name").write_text("package-0\n")
            (package / "energy_uj").write_text("90\n")
            (package / "max_energy_range_uj").write_text("100\n")
            domains = energy.discover_rapl(root)
            self.assertEqual(len(domains), 1)
            self.assertTrue(domains[0]["contributes"])
            self.assertEqual(domains[0]["max_energy_range_uj"], 100)

    def test_amd_hwmon_discovers_socket_without_double_counting_cores(self):
        with tempfile.TemporaryDirectory() as d:
            root = pathlib.Path(d)
            hwmon = root / "hwmon0"
            hwmon.mkdir()
            (hwmon / "name").write_text("amd_energy\n")
            (hwmon / "energy1_input").write_text("1000\n")
            (hwmon / "energy1_label").write_text("Ecore0\n")
            (hwmon / "energy2_input").write_text("5000\n")
            (hwmon / "energy2_label").write_text("Esocket0\n")
            domains = energy.discover_amd_hwmon_energy(root)
            self.assertEqual(len(domains), 2)
            contributing = [x for x in domains if x["contributes"]]
            self.assertEqual(len(contributing), 1)
            self.assertEqual(contributing[0]["name"], "Esocket0")
            self.assertEqual(contributing[0]["unit"], "microjoule")

    def test_probe_does_not_invent_evidence(self):
        with tempfile.TemporaryDirectory() as d:
            root = pathlib.Path(d)
            value = energy.probe(powercap_root=root, hwmon_root=root)
            self.assertIsNone(value["highest_available_evidence_class"])
            self.assertFalse(value["physical_system_energy_measured"])
            self.assertFalse(value["claim_authorized"])

    def test_profile_loader_ignores_invalid_files(self):
        with tempfile.TemporaryDirectory() as d:
            root = pathlib.Path(d)
            valid = profile("E3")
            valid["validated_calibration"] = False
            (root / "valid.json").write_text(json.dumps(valid))
            (root / "invalid.json").write_text("{}")
            loaded = energy.load_profiles(root)
            self.assertEqual(len(loaded), 1)
            self.assertEqual(loaded[0]["profile_id"], "unit-test-profile")


if __name__ == "__main__":
    unittest.main()
