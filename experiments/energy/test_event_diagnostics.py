"""Faults in PMU metadata must not become valid counts or energy claims."""
import copy
import json
from pathlib import Path
import tempfile
import unittest
import event_diagnostics as events
import generated_infer_evidence as generated
import test_generated_infer_evidence as fixtures


def boundary(available=True):
    return dict(scope="calling_thread_user_space", worker_threads_included=False, inherit=False,
                exclude_kernel=True, exclude_hv=True, cpu=-1, pid_argument=0, thread_id=7,
                observer_elapsed_ns=1000, counter_windows="independent_enable_disable",
                events=[dict(name=name, type=0, config=config, available=available,
                             reason="available" if available else "open_failed", errno=0 if available else 13,
                             event_id=i if available else None, raw_count=0 if available else None,
                             time_enabled_ns=100 if available else None,
                             time_running_ns=50 if available else None,
                             multiplexed=True if available else None, scaled_count_estimate=None)
                        for i, (name, config) in enumerate(events.EVENTS)])


def sample(batch=1, available=True):
    return dict(schema="shorthand.generated_infer.events.v1", success=True, diagnostic_only=True,
                instrumented=True, energy_claim_authorized=False, latency_claim_authorized=False,
                hardware_measured_joules=None, batch=batch, iterations=32, completed_calls=64,
                threads=1, warmups=8, cold_calls=1, empty_calls=0, empty_boundary=boundary(available),
                cold_boundary=boundary(available), warm_blocks=[boundary(available), boundary(available)])


class EventDiagnosticsTest(unittest.TestCase):
    def bundle(self, out, available=True):
        manifest = fixtures.GeneratedEvidenceTest().bundle(out)
        manifest.update(schema="shorthand.generated_infer.bundle.v3", event_profiles=[])
        for batch in (1, 16, 32):
            path = f"events-{batch}.json"
            generated.write(out / path, sample(batch, available))
            manifest["event_profiles"].append(dict(cell=f"b{batch}-t1", path=path))
            manifest["hashes"][path] = generated.sha(out / path)
        (out / "event-sources").mkdir()
        for name in events.SOURCES:
            path = "event-sources/" + name
            (out / path).write_text("synthetic source snapshot, not measurement evidence\n")
            manifest["hashes"][path] = generated.sha(out / path)
        return manifest

    def replay(self, out, manifest):
        generated.write(out / "manifest.json", manifest)
        return generated.replay(out, generated.sha(out / "manifest.json"))

    def test_available_zero_counts_and_unavailable_remain_distinct(self):
        for available in (True, False):
            events.validate_sample(sample(available=available), 1)
            with tempfile.TemporaryDirectory() as tmp:
                out = Path(tmp)
                self.replay(out, self.bundle(out, available))
                report = json.loads((out / "events.json").read_text())
                events.validate_report(report)
                self.assertTrue(report["success"])
                self.assertIsNone(report["hardware_measured_joules"])
                self.assertFalse(report["energy_claim_authorized"])
                self.assertEqual(len(report["rows"]), 3)
                self.assertIn("Diagnostic performance events", (out / "GENERATED.md").read_text())

    def test_rejects_scope_counts_or_claim_changes(self):
        for key, value in [("scope", "all_threads"), ("worker_threads_included", True),
                           ("inherit", True), ("exclude_kernel", False), ("cpu", 0),
                           ("thread_id", True), ("observer_elapsed_ns", 0), ("events", [])]:
            data = boundary(); data[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError): events.validate_boundary(data)
        for key, value in [("instrumented", False), ("hardware_measured_joules", 0),
                           ("energy_claim_authorized", True), ("latency_claim_authorized", True),
                           ("completed_calls", 63), ("threads", 2), ("warm_blocks", []),
                           ("cold_calls", 0), ("empty_calls", 1)]:
            data = sample(); data[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError): events.validate_sample(data, 1)
        data = sample(); data["warm_blocks"][1]["thread_id"] += 1
        with self.assertRaisesRegex(ValueError, "thread changed"): events.validate_sample(data, 1)

    def test_rejects_bad_events_and_scheduling(self):
        for key, value in [("name", "LLC_misses"), ("config", 123), ("type", True), ("event_id", None),
                           ("raw_count", True), ("raw_count", -1), ("raw_count", 1 << 64),
                           ("time_enabled_ns", 0), ("time_running_ns", 101), ("time_running_ns", 0),
                           ("multiplexed", False), ("scaled_count_estimate", 100), ("reason", []),
                           ("errno", 13), ("available", 1)]:
            data = boundary(); data["events"][0][key] = value
            with self.subTest(key=key, value=value), self.assertRaises(ValueError): events.validate_boundary(data)
        data = boundary(); data["events"][1]["event_id"] = data["events"][0]["event_id"]
        with self.assertRaisesRegex(ValueError, "duplicate"): events.validate_boundary(data)
        for key, value in [("raw_count", 0), ("time_enabled_ns", 100), ("reason", "available"), ("errno", 0)]:
            data = boundary(False); data["events"][0][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError): events.validate_boundary(data)

    def test_rejects_missing_duplicate_unhashed_or_control_records(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp); manifest = self.bundle(out)
            for key, value in [("event_profiles", []), ("event_profiles", [manifest["event_profiles"][0]] * 3)]:
                data = copy.deepcopy(manifest); data[key] = value
                with self.assertRaises(ValueError): self.replay(out, data)
            for path in ["missing.json", manifest["profiles"][0]["path"], manifest["observations"][0]["path"]]:
                data = copy.deepcopy(manifest); data["event_profiles"][0]["path"] = path
                with self.assertRaises(ValueError): self.replay(out, data)
            for name in events.SOURCES:
                data = copy.deepcopy(manifest); del data["hashes"]["event-sources/" + name]
                with self.assertRaisesRegex(ValueError, "source snapshot"): self.replay(out, data)

    def test_rejects_corrupted_raw_data_even_after_rehash(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp); manifest = self.bundle(out)
            path = manifest["event_profiles"][0]["path"]
            data = sample(); data["cold_boundary"]["events"][0]["time_running_ns"] = 0
            generated.write(out / path, data)
            with self.assertRaisesRegex(ValueError, "hash mismatch"): self.replay(out, manifest)
            manifest["hashes"][path] = generated.sha(out / path)
            with self.assertRaisesRegex(ValueError, "scheduling"): self.replay(out, manifest)

    def test_diagnostic_sample_cannot_enter_latency_series(self):
        with self.assertRaises(ValueError): generated.validate_sample(sample(), 1, 32, 2)

    def test_report_rejects_corrupted_metadata_and_measurements(self):
        for key, value in (("diagnostic_only", False), ("hardware_measured_joules", 1),
                           ("manifest_sha256", "wrong"), ("rows", [])):
            data = report_fixture(); data[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError): events.validate_report(data)
        data = report_fixture(); data["rows"][0]["sample"]["warm_blocks"][0]["worker_threads_included"] = True
        with self.assertRaises(ValueError): events.validate_report(data)


def report_fixture():
    return dict(schema="shorthand.generated_infer.event_report.v1", success=True, diagnostic_only=True,
                latency_claim_authorized=False, energy_claim_authorized=False, hardware_measured_joules=None,
                manifest_sha256="0" * 64, boundary="synthetic test fixture; not measured evidence",
                rows=[dict(cell=f"b{batch}-t1", sample=sample(batch, False)) for batch in (1, 16, 32)])


if __name__ == "__main__":
    unittest.main()
