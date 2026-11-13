#!/usr/bin/env python3
"""Tests for the frozen energy protocol (no hardware or ShortHand build required)."""
from __future__ import annotations

import copy
import json
import math
import pathlib
import shutil
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import frozen_energy_protocol as fp  # noqa: E402

PROTOCOL = fp.load_protocol(HERE / "protocol.json")
QUIET = {"cpu_model": "Test CPU", "logical_cpus": 4, "machine": "x86_64", "kernel": "6.0",
         "virtualized": False, "scaling_governors": ["performance"], "intel_no_turbo": "1",
         "smt_active": "0", "load_average_1m": 0.0}


def fake_powercap(root: pathlib.Path, energy: int = 1000, max_range: int = 10_000_000) -> None:
    pkg = root / "intel-rapl:0"
    sub = root / "intel-rapl:0:0"
    for directory, name in ((pkg, "package-0"), (sub, "core")):
        directory.mkdir(parents=True)
        (directory / "name").write_text(name + "\n")
        (directory / "energy_uj").write_text(f"{energy}\n")
        (directory / "max_energy_range_uj").write_text(f"{max_range}\n")


class RaplTests(unittest.TestCase):
    def test_only_package_domains_are_summed(self):
        with tempfile.TemporaryDirectory() as tmp:
            fake_powercap(pathlib.Path(tmp))
            source = fp.RaplSource(pathlib.Path(tmp))
            self.assertTrue(source.available)
            self.assertEqual(len(source.domains), 1)

    def test_wrap_is_handled_and_double_wrap_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            fake_powercap(pathlib.Path(tmp))
            source = fp.RaplSource(pathlib.Path(tmp))
            self.assertAlmostEqual(source.delta_joules([9_999_000], [1_000], 0.001), 2000 / 1e6)
            with self.assertRaises(RuntimeError):
                source.delta_joules([9_999_000], [1_000], 20.0)

    def test_missing_or_unreadable_counters_are_unavailable(self):
        self.assertFalse(fp.RaplSource(pathlib.Path("/nonexistent-powercap")).available)
        with tempfile.TemporaryDirectory() as tmp:
            fake_powercap(pathlib.Path(tmp))
            (pathlib.Path(tmp) / "intel-rapl:0/energy_uj").write_text("garbage")
            self.assertFalse(fp.RaplSource(pathlib.Path(tmp)).available)

    def test_ac_meter_rejects_backwards_and_bad_values(self):
        meter = fp.AcMeterSource("echo nan")
        self.assertFalse(meter.available)
        self.assertTrue(fp.AcMeterSource("echo 5").available)
        with self.assertRaises(RuntimeError):
            fp.AcMeterSource("echo 5").delta_joules(5.0, 4.0, 1.0)


class QualificationTests(unittest.TestCase):
    def qualify(self, facts, rapl_ok=True, meter=None):
        original = fp.host_facts
        fp.host_facts = lambda: copy.deepcopy(facts)
        try:
            with tempfile.TemporaryDirectory() as tmp:
                if rapl_ok:
                    fake_powercap(pathlib.Path(tmp))
                return fp.qualify_host(PROTOCOL, fp.RaplSource(pathlib.Path(tmp)), fp.AcMeterSource(meter))
        finally:
            fp.host_facts = original

    def test_clean_host_qualifies(self):
        self.assertTrue(self.qualify(QUIET)["qualified"])

    def test_no_physical_source_never_qualifies(self):
        report = self.qualify(QUIET, rapl_ok=False)
        self.assertFalse(report["qualified"])
        self.assertTrue(any("no physical energy source" in r for r in report["disqualifying_reasons"]))

    def test_virtualised_noisy_or_unfixed_governor_hosts_are_rejected(self):
        for change in ({"virtualized": True}, {"load_average_1m": 3.0},
                       {"scaling_governors": ["powersave"]}, {"scaling_governors": ["unavailable"]}):
            facts = {**QUIET, **change}
            self.assertFalse(self.qualify(facts)["qualified"], change)


class StatisticsTests(unittest.TestCase):
    def test_bootstrap_is_deterministic_and_brackets_geomean(self):
        ratios = [0.9, 0.95, 1.0, 0.92, 0.97] * 6
        a = fp.bootstrap_ratio_ci(ratios, 0.95, 1000, 104)
        b = fp.bootstrap_ratio_ci(ratios, 0.95, 1000, 104)
        self.assertEqual(a, b)
        self.assertLessEqual(a["lower"], a["geomean"])
        self.assertLessEqual(a["geomean"], a["upper"])
        self.assertAlmostEqual(a["geomean"], math.exp(sum(map(math.log, ratios)) / len(ratios)))

    def test_invalid_ratios_rejected(self):
        for bad in ([], [0.0], [-1.0], [float("inf")]):
            with self.assertRaises(ValueError):
                fp.bootstrap_ratio_ci(bad, 0.95, 10, 1)

    def test_idle_power_is_median(self):
        self.assertEqual(fp.idle_power_watts([(10, 1), (20, 1), (1000, 1)]), 20)

    def test_claim_decisions(self):
        ci = lambda lo, hi, n=30: {"lower": lo, "upper": hi, "geomean": (lo + hi) / 2, "n": n}  # noqa: E731
        self.assertEqual(fp.decide_claim(PROTOCOL, ci(0.8, 0.95), True), "candidate_lower_energy")
        self.assertEqual(fp.decide_claim(PROTOCOL, ci(1.05, 1.2), True), "candidate_higher_energy")
        self.assertEqual(fp.decide_claim(PROTOCOL, ci(0.9, 1.1), True), "no_significant_difference")
        self.assertEqual(fp.decide_claim(PROTOCOL, ci(0.8, 0.9, 5), True), "inconclusive_insufficient_rounds")
        self.assertEqual(fp.decide_claim(PROTOCOL, ci(0.8, 0.9), False), "invalid_outputs_differ")

    def test_protocol_validation(self):
        broken = {k: v for k, v in PROTOCOL.items() if k != "seed"}
        with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False) as handle:
            json.dump(broken, handle)
        with self.assertRaises(ValueError):
            fp.load_protocol(pathlib.Path(handle.name))


def host_run(host: str, verdict: str = "candidate_lower_energy", qualified: bool = True, scale_full: bool = True,
             protocol_sha: str | None = None) -> dict:
    return {
        "schema": fp.SCHEMA, "kind": "host_run", "host_id": host, "host_qualified": qualified,
        "evidence_class": "physical_counter" if scale_full else None,
        "protocol_sha256": protocol_sha or fp.sha256_file(HERE / "protocol.json"),
        "workloads": [{"workload": "chain", "source_agreement": None, "summary": {
            "candidate_vs_baseline_energy_ratio": {"cpp17_gxx_O2": {"rapl": {"verdict": verdict}}}}}],
    }


class AggregateTests(unittest.TestCase):
    def aggregate(self, runs):
        with tempfile.TemporaryDirectory() as tmp:
            paths = []
            for index, run in enumerate(runs):
                path = pathlib.Path(tmp) / f"r{index}.json"
                path.write_text(json.dumps(run))
                paths.append(str(path))
            out = pathlib.Path(tmp) / "agg.json"
            fp.main(["aggregate", "--runs", *paths, "--output", str(out)])
            return json.loads(out.read_text())

    def test_two_qualified_hosts_with_wins_authorise_bounded_claim(self):
        agg = self.aggregate([host_run("a"), host_run("b")])
        self.assertTrue(agg["energy_claim_authorized"])

    def test_single_host_is_insufficient(self):
        self.assertFalse(self.aggregate([host_run("a")])["energy_claim_authorized"])

    def test_duplicate_host_counts_once(self):
        self.assertFalse(self.aggregate([host_run("a"), host_run("a")])["energy_claim_authorized"])

    def test_any_loss_or_tie_blocks_claim(self):
        for verdict in ("candidate_higher_energy", "no_significant_difference", "inconclusive_insufficient_rounds"):
            agg = self.aggregate([host_run("a"), host_run("b", verdict)])
            self.assertFalse(agg["energy_claim_authorized"], verdict)

    def test_unqualified_smoke_or_changed_protocol_blocks_claim(self):
        self.assertFalse(self.aggregate([host_run("a"), host_run("b", qualified=False)])["energy_claim_authorized"])
        self.assertFalse(self.aggregate([host_run("a"), host_run("b", scale_full=False)])["energy_claim_authorized"])
        self.assertFalse(self.aggregate([host_run("a"), host_run("b", protocol_sha="0" * 64)])[
            "energy_claim_authorized"])


@unittest.skipUnless(shutil.which("gcc") and shutil.which("g++"), "gcc/g++ required")
class EndToEndTests(unittest.TestCase):
    """Runs the real harness with C vs C++ and a synthetic power meter."""

    def test_dry_run_and_synthetic_physical_run(self):
        protocol = copy.deepcopy(PROTOCOL)
        protocol.update({"candidate_variant": "c11_gcc_O2", "rounds": 4, "warmup_rounds": 1, "idle_windows": 2,
                         "bootstrap_resamples": 200, "minimum_rounds_for_claim": 4})
        protocol["variants"] = {k: v for k, v in protocol["variants"].items() if k in ("c11_gcc_O2", "cpp17_gxx_O2")}
        protocol["workloads"]["chain"]["n"] = 2_000_000
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            proto_path = tmp / "protocol.json"
            proto_path.write_text(json.dumps(protocol))
            meter = tmp / "meter.py"
            # Synthetic cumulative meter: 10 W idle floor plus 0.5 J per busy CPU jiffy system-wide.
            meter.write_text("import time\nf = open('/proc/stat').readline().split()[1:4]\n"
                             "print(time.time() * 10.0 + sum(map(int, f)) * 0.5)\n")

            # 1. No physical source here (no powercap root): refused without --allow-unqualified.
            rc = fp.main(["run", "--protocol", str(proto_path), "--powercap-root", str(tmp / "none"),
                          "--output-dir", str(tmp / "refused")])
            self.assertEqual(rc, 3)
            self.assertFalse((tmp / "refused" / "run.json").exists())

            # 2. Dry run: outputs checked, time recorded, joules null, never authorised.
            rc = fp.main(["run", "--protocol", str(proto_path), "--powercap-root", str(tmp / "none"),
                          "--output-dir", str(tmp / "dry"), "--allow-unqualified"])
            self.assertEqual(rc, 0)
            dry = json.loads((tmp / "dry" / "run.json").read_text())
            self.assertIsNone(dry["evidence_class"])
            self.assertFalse(dry["energy_claim_authorized"])
            self.assertEqual(dry["energy_sources"], [])
            self.assertTrue(dry["workloads"][0]["outputs_equivalent"])

            # 3. Synthetic quiet host with an AC meter: full pipeline, scale 1.0.
            original = fp.host_facts
            fp.host_facts = lambda: dict(QUIET)
            try:
                rc = fp.main(["run", "--protocol", str(proto_path), "--powercap-root", str(tmp / "none"),
                              "--ac-meter-cmd", f"{sys.executable} {meter}", "--idle-seconds", "0.05",
                              "--output-dir", str(tmp / "phys")])
            finally:
                fp.host_facts = original
            self.assertEqual(rc, 0)
            phys = json.loads((tmp / "phys" / "run.json").read_text())
            self.assertEqual(phys["evidence_class"], "physical_counter")
            self.assertFalse(phys["energy_claim_authorized"])
            ratio = phys["workloads"][0]["summary"]["candidate_vs_baseline_energy_ratio"]["cpp17_gxx_O2"]["ac_meter"]
            self.assertIn("verdict", ratio)
            self.assertEqual(ratio["n"], 4)
            # One host can never authorise a claim, whatever the ratio.
            agg = self.aggregate_single(tmp, proto_path, tmp / "phys" / "run.json")
            self.assertFalse(agg["energy_claim_authorized"])

    @staticmethod
    def aggregate_single(tmp, proto_path, run_path):
        out = tmp / "agg.json"
        fp.main(["aggregate", "--protocol", str(proto_path), "--runs", str(run_path), "--output", str(out)])
        return json.loads(out.read_text())


if __name__ == "__main__":
    unittest.main()
