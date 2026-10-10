#!/usr/bin/env python3
"""Negative controls for candidate completeness and receipt provenance."""
import hashlib
import importlib.util
import json
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("candidates", ROOT / "scripts/verify_release_candidates.py")
candidates = importlib.util.module_from_spec(spec)
spec.loader.exec_module(candidates)
spec = importlib.util.spec_from_file_location("installed", ROOT / "scripts/check_installed_cpu_package.py")
installed = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installed)


class Candidates(unittest.TestCase):
    def test_source_checkout_rejects_normalization_mismatch_and_actual_edits(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            def git(*args):
                return subprocess.check_output(["git", "-C", str(root), *args], text=True, stderr=subprocess.PIPE).strip()
            git("init")
            git("config", "user.name", "Package test")
            git("config", "user.email", "package-test@example.invalid")
            git("config", "core.autocrlf", "false")
            source = root / "source.txt"
            source.write_bytes(b"original\n")
            git("add", "source.txt")
            git("-c", "commit.gpgsign=false", "commit", "-m", "fixture")
            revision = git("rev-parse", "HEAD")
            self.assertEqual(installed.clean_source_revision(root), revision)
            # A Windows Git checkout using autocrlf=true creates these bytes,
            # which another Git using autocrlf=false must report as modified.
            source.write_bytes(b"original\r\n")
            with self.assertRaisesRegex(RuntimeError, "clean tracked source.*|source.txt"):
                installed.clean_source_revision(root)
            git("checkout", "--", "source.txt")
            self.assertEqual(source.read_bytes(), b"original\n")
            self.assertEqual(installed.clean_source_revision(root), revision)
            (root / "untracked-build.log").write_text("build output")
            self.assertEqual(installed.clean_source_revision(root), revision)
            source.write_bytes(b"actual source change\n")
            with self.assertRaisesRegex(RuntimeError, "source.txt"):
                installed.clean_source_revision(root)
            git("add", "source.txt")
            with self.assertRaisesRegex(RuntimeError, "source.txt"):
                installed.clean_source_revision(root)

    def test_selection_requires_all_bundles_and_ignores_policy_report(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            (root / "release-closeout-policy").mkdir()
            for platform in candidates.PLATFORMS:
                (root / ("release-bundle-" + platform)).mkdir()
            self.assertEqual(len(candidates.bundle_directories(root)), 4)
            (root / "release-bundle-windows-x64").rmdir()
            with self.assertRaisesRegex(ValueError, "missing="):
                candidates.bundle_directories(root)
            (root / "release-bundle-windows-x64").mkdir()
            (root / "release-bundle-unknown").mkdir()
            with self.assertRaisesRegex(ValueError, "unexpected="):
                candidates.bundle_directories(root)

    def test_receipt_must_match_actual_archive_revision_and_native_execution(self):
        with tempfile.TemporaryDirectory() as temporary:
            bundle = pathlib.Path(temporary)
            (bundle / "test.tar").write_bytes(b"fixture archive")
            digest = hashlib.sha256(b"fixture archive").hexdigest()
            revision = "a" * 40
            (bundle / "test.manifest").write_text(f"artifact=test.tar\nartifact_sha256={digest}\ncommit={revision}\nplatform=windows-x64\n")
            report = dict(schema="shorthand.release.cpu_package.v1", status="pass", platform="windows-x64",
                          native_os="Windows", architecture="amd64", archive_sha256=digest, source_revision=revision,
                          onnxruntime_version="1.30.0", production_claim=False, source_dirty=False,
                          qualification_mode="experimental_native_candidate", checks=sorted(candidates.CHECKS))
            path = bundle / "test.cpu-qualification.json"
            path.write_text(json.dumps(report))
            candidates.verify_receipt(bundle, "windows-x64", revision)
            mutations = {"status": "skip", "platform": "linux-x64", "native_os": "Linux", "architecture": "arm64",
                         "source_revision": "b" * 40, "archive_sha256": "0" * 64, "checks": [],
                         "onnxruntime_version": "0.0.0", "production_claim": True, "source_dirty": True,
                         "qualification_mode": "existing_production_scope"}
            for key, value in mutations.items():
                with self.subTest(field=key):
                    path.write_text(json.dumps({**report, key: value}))
                    with self.assertRaisesRegex(ValueError, "qualification"):
                        candidates.verify_receipt(bundle, "windows-x64", revision)
            path.write_text(json.dumps(report))
            (bundle / "test.tar").write_bytes(b"tampered archive")
            with self.assertRaisesRegex(ValueError, "qualification"):
                candidates.verify_receipt(bundle, "windows-x64", revision)
            path.unlink()
            with self.assertRaisesRegex(ValueError, "require one"):
                candidates.verify_receipt(bundle, "windows-x64", revision)


if __name__ == "__main__":
    unittest.main()
