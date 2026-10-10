#!/usr/bin/env python3
"""Require exactly the four native bundles and their archive-bound CPU receipts."""
import argparse
import hashlib
import json
import pathlib
import subprocess

PLATFORMS = ("linux-x64", "linux-arm64", "macos-arm64", "windows-x64")
CHECKS = {"relocated_archive", "build_and_sdk_hidden", "installed_clis", "static_cpu_numerics",
          "shared_cpu_numerics", "compiled_core_source", "interpreter_cpu", "nonfinite_rollback",
          "missing_runtime_rejected", "production_scope_guard"}
ROOT = pathlib.Path(__file__).resolve().parents[1]


def verify_receipt(bundle, platform, revision):
    manifests = list(bundle.glob("*.manifest"))
    if len(manifests) != 1:
        raise ValueError(f"{platform}: require one manifest")
    manifest = dict(line.split("=", 1) for line in manifests[0].read_text().splitlines())
    artifact = manifest["artifact"]
    if pathlib.Path(artifact).name != artifact or ".." in artifact:
        raise ValueError("unsafe archive name")
    reports = list(bundle.glob("*.cpu-qualification.json"))
    if len(reports) != 1:
        raise ValueError(f"{platform}: require one CPU qualification receipt")
    report = json.loads(reports[0].read_text())
    digest = hashlib.sha256((bundle / artifact).read_bytes()).hexdigest()
    expected_os = "Windows" if platform.startswith("windows") else "Darwin" if platform.startswith("macos") else "Linux"
    expected_arch = {"arm64", "aarch64"} if platform.endswith("arm64") else {"x86_64", "amd64"}
    expected_mode = "existing_production_scope" if platform == "linux-x64" else "experimental_native_candidate"
    if (report.get("schema") != "shorthand.release.cpu_package.v1" or report.get("status") != "pass"
            or report.get("platform") != platform or manifest.get("platform") != platform
            or report.get("native_os") != expected_os or report.get("architecture") not in expected_arch
            or report.get("archive_sha256") != digest or manifest.get("artifact_sha256") != digest
            or report.get("source_revision") != revision or manifest.get("commit") != revision
            or report.get("source_dirty") is not False
            or report.get("qualification_mode") != expected_mode
            or report.get("onnxruntime_version") != "1.30.0"
            or report.get("production_claim") is not False
            or not CHECKS.issubset(report.get("checks", []))):
        raise ValueError(f"{platform}: missing, mismatched or stale CPU qualification")


def bundle_directories(root):
    expected = {"release-bundle-" + platform for platform in PLATFORMS}
    actual = {p.name for p in root.glob("release-bundle-*") if p.is_dir()}
    if actual != expected:
        raise ValueError(f"release bundle set mismatch: missing={sorted(expected - actual)}, unexpected={sorted(actual - expected)}")
    return [(platform, root / ("release-bundle-" + platform)) for platform in PLATFORMS]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=pathlib.Path)
    parser.add_argument("--revision", required=True)
    args = parser.parse_args()
    for platform, bundle in bundle_directories(args.directory):
        subprocess.run(["bash", str(ROOT / "scripts/verify_release_bundle.sh"), str(bundle)], check=True)
        verify_receipt(bundle, platform, args.revision)
    print("PASS four release bundles and native CPU qualification receipts")


if __name__ == "__main__":
    main()
