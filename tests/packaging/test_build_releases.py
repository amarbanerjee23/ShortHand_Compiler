#!/usr/bin/env python3
"""Fail-closed build release and installer receipt controls."""
import hashlib
import json
import pathlib
import sys
import shutil
import subprocess
from unittest import mock
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import publish_build_release as release
import verify_release_candidates as candidates
import build_native_installer as installers


class BuildReleases(unittest.TestCase):
    def test_nsis_paths_keep_native_separators_and_unquoted_space_overrides(self):
        exe = "C:/release files/setup.exe"
        prefix = "C:/temporary files/installed with spaces"
        expected_install = r'"C:\release files\setup.exe" /S /D=C:\temporary files\installed with spaces'
        expected_uninstall = r'"C:\release files\setup.exe" /S _?=C:\temporary files\installed with spaces'
        original_str = pathlib.PureWindowsPath.__str__
        # MSYS2 Python 3.14 patches ntpath.sep; PureWindowsPath prints '/' too.
        msys_str = lambda p: original_str(p).replace("\\", "/")
        for formatter in (original_str, msys_str):
            with mock.patch.object(pathlib.PureWindowsPath, "__str__", formatter):
                for uninstall, expected in ((False, expected_install), (True, expected_uninstall)):
                    command = installers.nsis_command(exe, prefix, uninstall=uninstall)
                    self.assertEqual(command, expected)
                    self.assertEqual(command.count('"'), 2)
                self.assertEqual(installers.nsis_path("bin/nested/$value.dll"), r"bin\nested\$$value.dll")
        with self.assertRaises(ValueError):
            installers.nsis_command(exe, "relative/path")

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

    def test_upload_corruption_rolls_back_draft_and_never_publishes(self):
        for failure in (None, "corrupt", "upload"):
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as temporary:
                root = pathlib.Path(temporary)
                for platform in candidates.PLATFORMS:
                    folder = root / ("release-bundle-" + platform)
                    folder.mkdir()
                    (folder / (platform + ".tar")).write_bytes(platform.encode())
                calls, assets = [], []

                def fake_gh(*args, check=True):
                    calls.append(args)
                    if args[:2] == ("release", "view") or (args[0] == "api" and not check):
                        return subprocess.CompletedProcess(args, 1, "", "not found")
                    if args[:2] == ("release", "upload"):
                        if failure == "upload":
                            raise RuntimeError("injected upload failure")
                        assets.extend(pathlib.Path(p) for p in args[3:args.index("--repo")])
                    if args[:2] == ("release", "download"):
                        folder = pathlib.Path(args[args.index("--dir") + 1])
                        for asset in assets:
                            shutil.copyfile(asset, folder / asset.name)
                        if failure == "corrupt":
                            (folder / assets[0].name).write_bytes(b"corrupt")
                    return subprocess.CompletedProcess(args, 0, json.dumps({"object": {"sha": "a" * 40}}), "")

                with mock.patch.object(release, "gh", side_effect=fake_gh), \
                     mock.patch.object(release.subprocess, "run") as verify, \
                     mock.patch.dict(release.os.environ, {"GITHUB_RUN_ID": "123"}):
                    if failure:
                        with self.assertRaises((ValueError, RuntimeError)):
                            release.publish(root, "build-123-1-aaaaaaaaaaaa", "a" * 40)
                    else:
                        release.publish(root, "build-123-1-aaaaaaaaaaaa", "a" * 40)
                    self.assertIn("verify_release_candidates.py", verify.call_args.args[0][1])
                self.assertEqual(any(c[:2] == ("release", "delete") for c in calls), bool(failure))
                self.assertEqual(any(c[:2] == ("release", "edit") for c in calls), not failure)
                creation = next(c for c in calls if c[:2] == ("release", "create"))
                self.assertIn("--draft", creation)
                self.assertIn("--verify-tag", creation)
                self.assertIn("--prerelease", creation)
                self.assertIn("--latest=false", creation)

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
