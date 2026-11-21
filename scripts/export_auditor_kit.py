#!/usr/bin/env python3
"""Export a self-contained, SHA-256-indexed External Auditor Verification Kit.

Packages the repository's closeout report, production truth matrix, release
ledger, known limitations, and frozen energy protocol manifest into a single
offline-verifiable directory with a cryptographic manifest (SHA256SUMS.json).
"""
from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCHEMA = "shorthand.external_auditor_kit.v1"

CORE_ARTIFACTS = (
    "docs/production_truth.tsv",
    "docs/release_closeout_ledger.json",
    "docs/release_claims_closeout.md",
    "docs/enterprise_release_scorecard.md",
    "docs/known_limitations.md",
    "docs/FROZEN_ENERGY_PROTOCOL.md",
    "experiments/energy/frozen_protocol/protocol.json",
)


def sha256_file(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def export_kit(root: pathlib.Path, output_dir: pathlib.Path, revision: str | None = None) -> dict[str, Any]:
    root = root.resolve()
    output_dir = output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    artifacts_dir = output_dir / "artifacts"
    artifacts_dir.mkdir(parents=True, exist_ok=True)

    if revision is None:
        proc = subprocess.run(
            ["git", "-C", str(root), "rev-parse", "HEAD"],
            capture_output=True, text=True, check=False,
        )
        revision = proc.stdout.strip() or "unknown"

    copied: dict[str, dict[str, Any]] = {}
    for rel in CORE_ARTIFACTS:
        src = root / rel
        if not src.exists():
            raise FileNotFoundError(f"required audit artifact missing: {src}")
        dst = artifacts_dir / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        copied[rel] = {"sha256": sha256_file(dst), "bytes": dst.stat().st_size}

    closeout_out = output_dir / "release_closeout_report.json"
    closeout_script = root / "scripts" / "release_closeout.py"
    if closeout_script.exists():
        subprocess.run(
            [sys.executable, str(closeout_script), "--root", str(root),
             "--revision", revision, "--report", str(closeout_out)],
            check=True, capture_output=True, text=True,
        )
        copied["release_closeout_report.json"] = {
            "sha256": sha256_file(closeout_out),
            "bytes": closeout_out.stat().st_size,
        }

    manifest = {
        "schema": SCHEMA,
        "generated_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "revision": revision,
        "artifact_count": len(copied),
        "artifacts": copied,
    }
    (output_dir / "SHA256SUMS.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    return manifest


def verify_kit(kit_dir: pathlib.Path) -> tuple[bool, list[str]]:
    kit_dir = kit_dir.resolve()
    manifest_path = kit_dir / "SHA256SUMS.json"
    if not manifest_path.exists():
        return False, ["missing SHA256SUMS.json"]
    if manifest_path.is_symlink():
        return False, ["unsafe manifest"]
    try:
        manifest = json.loads(manifest_path.read_text())
    except (OSError, ValueError):
        return False, ["invalid manifest JSON"]
    if not isinstance(manifest, dict):
        return False, ["invalid manifest object"]
    if manifest.get("schema") != SCHEMA:
        return False, [f"unexpected schema: {manifest.get('schema')}"]
    errors: list[str] = []
    artifacts = manifest.get("artifacts")
    required = set(CORE_ARTIFACTS) | {"release_closeout_report.json"}
    if not isinstance(artifacts, dict) or set(artifacts) != required or manifest.get("artifact_count") != len(required):
        return False, ["incomplete or unexpected artifact inventory"]
    for rel, meta in artifacts.items():
        target = kit_dir / rel if rel == "release_closeout_report.json" else kit_dir / "artifacts" / rel
        if not isinstance(meta, dict):
            errors.append(f"invalid metadata: {rel}")
            continue
        if any(parent.is_symlink() for parent in [target, *target.parents] if parent != kit_dir.parent) or not target.resolve().is_relative_to(kit_dir):
            errors.append(f"unsafe artifact: {rel}")
            continue
        if not target.is_file():
            errors.append(f"missing artifact: {rel}")
            continue
        if type(meta.get("bytes")) is not int or target.stat().st_size != meta["bytes"]:
            errors.append(f"size mismatch: {rel}")
        actual = sha256_file(target)
        if actual != meta.get("sha256"):
            errors.append(f"checksum mismatch for {rel}: expected {meta.get('sha256')}, got {actual}")
    return not errors, errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    exp = sub.add_parser("export", help="Export offline-verifiable auditor kit")
    exp.add_argument("--root", type=pathlib.Path, default=ROOT)
    exp.add_argument("--output-dir", type=pathlib.Path, required=True)
    exp.add_argument("--revision", default=None)

    ver = sub.add_parser("verify", help="Verify an exported auditor kit")
    ver.add_argument("--kit-dir", type=pathlib.Path, required=True)

    args = parser.parse_args(argv)
    if args.command == "export":
        manifest = export_kit(args.root, args.output_dir, args.revision)
        print(json.dumps({"status": "exported", "artifacts": manifest["artifact_count"],
                          "output_dir": str(args.output_dir)}))
        return 0
    if args.command == "verify":
        ok, errs = verify_kit(args.kit_dir)
        print(json.dumps({"status": "pass" if ok else "fail", "errors": errs}))
        return 0 if ok else 1
    return 2


if __name__ == "__main__":
    sys.exit(main())
