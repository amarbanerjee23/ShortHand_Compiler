#!/usr/bin/env python3
"""Replay and adversarial evidence checks; synthetic fixtures are not measurements."""
import argparse
import copy
import json
from pathlib import Path
import tempfile
import unittest
import training_runtime_delta as evidence


def rows():
    result = []
    for threads in (1, 2):
        for mode in ("base", "candidate"):
            for kind, count in (("warmup", 2), ("latency", 6), ("allocations", 1), ("injected", 1), ("seed-check", 1)):
                for iteration in range(count):
                    counted = kind in ("allocations", "injected")
                    result.append(dict(threads=threads, mode=mode, kind=kind, iteration=iteration,
                        seed=7 if kind == "seed-check" else 42, seconds=1.0, final_loss=0.1,
                        initial_loss=0.7, accuracy=1.0, samples=1024, steps=128, joules=None,
                        parameters_sha256="a"*64, allocations=(11 if kind == "injected" else 10) if counted else None,
                        allocated_bytes=(164 if kind == "injected" else 100) if counted else None))
    return result


class EvidenceTests(unittest.TestCase):
    def test_rejects_corrupt_observations(self):
        valid = rows()
        evidence.validate(valid)
        changes = [(0, "joules", 0), (0, "seconds", float("nan")), (0, "samples", 1023),
                   (0, "parameters_sha256", "b"*64), (0, "seed", 7), (0, "accuracy", 0.9),
                   (8, "allocations", True), (9, "allocated_bytes", 163)]
        for index, key, value in changes:
            altered = copy.deepcopy(valid); altered[index][key] = value
            with self.subTest(key=key), self.assertRaises(AssertionError):
                evidence.validate(altered)
        for altered in (valid[:-1], valid[:-1] + [valid[0]]):
            with self.assertRaises(AssertionError): evidence.validate(altered)

    def test_hashed_bundle(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            for name in evidence.REQUIRED:
                path = folder / name; path.parent.mkdir(parents=True, exist_ok=True); path.write_text("fixture")
            (folder / "observations.json").write_text(json.dumps(rows()))
            (folder / "report.json").write_text(json.dumps(evidence.report(rows())))
            manifest = dict(schema="shorthand.training_delta.bundle.v1", base="a"*40, head="b"*40,
                files={name: evidence.digest(folder / name) for name in evidence.REQUIRED})
            def write_manifest():
                (folder / "manifest.json").write_text(json.dumps(manifest))
                return evidence.digest(folder / "manifest.json")
            expected = write_manifest(); evidence.replay(folder, expected)
            with self.assertRaises(AssertionError): evidence.replay(folder, "0"*64)
            del manifest["files"]["sources/ai_runtime/training/PersistentWorkers.h"]
            with self.assertRaises(AssertionError): evidence.replay(folder, write_manifest())
            manifest["files"]["sources/ai_runtime/training/PersistentWorkers.h"] = evidence.digest(folder / "sources/ai_runtime/training/PersistentWorkers.h")
            (folder / "candidate.cpp").write_text("tampered")
            with self.assertRaises(AssertionError): evidence.replay(folder, write_manifest())
            manifest["files"]["candidate.cpp"] = evidence.digest(folder / "candidate.cpp")
            report = evidence.report(rows()); report["energy_claim_authorized"] = True
            (folder / "report.json").write_text(json.dumps(report))
            manifest["files"]["report.json"] = evidence.digest(folder / "report.json")
            with self.assertRaises(AssertionError): evidence.replay(folder, write_manifest())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(); parser.add_argument("--bundle", type=Path)
    args, extra = parser.parse_known_args()
    if args.bundle:
        evidence.replay(args.bundle, (args.bundle / "manifest.sha256").read_text().strip())
    unittest.main(argv=[__file__, *extra])
