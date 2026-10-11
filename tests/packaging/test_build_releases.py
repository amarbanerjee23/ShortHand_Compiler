#!/usr/bin/env python3
"""Fail-closed build release and installer receipt controls."""
import hashlib
import json
import pathlib
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import publish_build_release as release
import verify_release_candidates as candidates


class BuildReleases(unittest.TestCase):
    def test_only_trusted_pushes_have_immutable_release_identity(self):
        env = dict(GITHUB_EVENT_NAME="push", GITHUB_REPOSITORY=release.REPOSITORY,
                   GITHUB_REF="refs/heads/feature", GITHUB_SHA="a" * 40,
                   GITHUB_RUN_ID="123", GITHUB_RUN_ATTEMPT="1")
        tag, revision = release.release_identity(env)
        self.assertEqual(tag, "build-123-1-aaaaaaaaaaaa")
        self.assertEqual(revision, env["GITHUB_SHA"])
        self.assertNotEqual(tag, release.release_identity({**env, "GITHUB_RUN_ATTEMPT": "2"})[0])
        for key, value in (("GITHUB_EVENT_NAME", "pull_request"), ("GITHUB_EVENT_NAME", "pull_request_target"),
                           ("GITHUB_EVENT_NAME", "workflow_run"), ("GITHUB_REPOSITORY", "fork/repo"),
                           ("GITHUB_REF", "refs/tags/v1.0.0"), ("GITHUB_SHA", "master"),
                           ("GITHUB_RUN_ID", "1;cmd"), ("GITHUB_RUN_ATTEMPT", "0")):
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                release.release_identity({**env, key: value})

    def test_installer_requires_native_lifecycle_and_exact_bytes(self):
        for platform in candidates.PLATFORMS:
            with self.subTest(platform=platform), tempfile.TemporaryDirectory() as temporary:
                bundle = pathlib.Path(temporary)
                suffix = ".exe" if platform.startswith("windows") else ".pkg" if platform.startswith("macos") else ".deb"
                name = "fixture-setup" + suffix
                installer = bundle / name
                installer.write_bytes(b"qualified installer")
                digest = hashlib.sha256(installer.read_bytes()).hexdigest()
                (bundle / (name + ".SHA256SUMS")).write_text(f"{digest}  {name}\n")
                (bundle / "fixture.manifest").write_text("artifact_sha256=" + "b" * 64 + "\n")
                report = dict(schema="shorthand.release.installer.v1", status="pass", platform=platform,
                              source_revision="a" * 40, archive_sha256="b" * 64, installer=name,
                              installer_sha256=digest, production_claim=False,
                              checks=["native_install", "exact_archive_payload", "installed_cpu_suite", "native_uninstall"])
                path = bundle / "fixture.installer-qualification.json"
                path.write_text(json.dumps(report))
                candidates.verify_installer(bundle, platform, "a" * 40)
                for key, value in (("status", "skip"), ("platform", "unknown"), ("production_claim", True),
                                   ("source_revision", "c" * 40), ("archive_sha256", "d" * 64),
                                   ("installer_sha256", "e" * 64), ("installer", "../" + name),
                                   ("checks", ["native_install", "exact_archive_payload", "installed_cpu_suite"])):
                    with self.subTest(key=key), self.assertRaises(ValueError):
                        path.write_text(json.dumps({**report, key: value}))
                        candidates.verify_installer(bundle, platform, "a" * 40)
                path.write_text(json.dumps(report))
                installer.write_bytes(b"tampered installer")
                with self.assertRaises(ValueError):
                    candidates.verify_installer(bundle, platform, "a" * 40)
                path.unlink()
                with self.assertRaises(ValueError):
                    candidates.verify_installer(bundle, platform, "a" * 40)


if __name__ == "__main__":
    unittest.main()
