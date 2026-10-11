#!/usr/bin/env python3
"""Publish a complete, verified push build as an immutable development prerelease."""
import argparse
import hashlib
import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile

from verify_release_candidates import bundle_directories

ROOT = pathlib.Path(__file__).resolve().parents[1]
REPOSITORY = "amarbanerjee23/ShortHand_Compiler"


def release_identity(env):
    if (env.get("GITHUB_EVENT_NAME") != "push" or env.get("GITHUB_REPOSITORY") != REPOSITORY
            or not env.get("GITHUB_REF", "").startswith("refs/heads/")):
        raise ValueError("only trusted repository branch pushes may publish build releases")
    sha = env.get("GITHUB_SHA", "")
    run_id, attempt = env.get("GITHUB_RUN_ID", ""), env.get("GITHUB_RUN_ATTEMPT", "")
    if not re.fullmatch(r"[a-f0-9]{40}", sha) or not re.fullmatch(r"[1-9][0-9]*", run_id) or not re.fullmatch(r"[1-9][0-9]*", attempt):
        raise ValueError("invalid build identity")
    return f"build-{run_id}-{attempt}-{sha[:12]}", sha


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def gh(*args, check=True):
    return subprocess.run(["gh", *map(str, args)], check=check, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=900)


def publish(root, tag, revision):
    subprocess.run([sys.executable, str(ROOT / "scripts/verify_release_candidates.py"),
                    str(root), "--revision", revision], check=True)
    assets = [p for _, directory in bundle_directories(root) for p in sorted(directory.iterdir())]
    if any(not p.is_file() or p.is_symlink() or not re.fullmatch(r"[A-Za-z0-9._-]+", p.name) for p in assets):
        raise ValueError("unsafe build asset")
    if len({p.name for p in assets}) != len(assets):
        raise ValueError("duplicate release asset basenames")
    hashes = {p.name: digest(p) for p in assets}
    checksums = root / "BUILD_SHA256SUMS"
    checksums.write_text("".join(f"{value}  {name}\n" for name, value in sorted(hashes.items())))
    assets.append(checksums)
    hashes[checksums.name] = digest(checksums)
    if gh("release", "view", tag, "--repo", REPOSITORY, check=False).returncode == 0:
        raise ValueError("build release already exists; immutable releases are never overwritten")
    if gh("api", f"repos/{REPOSITORY}/git/ref/tags/{tag}", check=False).returncode == 0:
        raise ValueError("build tag already exists; refusing to reuse it")
    notes = root / "BUILD_NOTES.md"
    notes.write_text(f"""Development build of `{revision}`.

All mandatory CI jobs and four native installer lifecycle tests passed.

| System | Installer |
| --- | --- |
| Windows x64 (MSYS2 UCRT64) | `*-windows-x64-setup.exe` |
| Ubuntu 24.04 x64 | `*-linux-x64-setup.deb` |
| Ubuntu 24.04 ARM64 | `*-linux-arm64-setup.deb` |
| macOS 15 Apple Silicon | `*-macos-arm64-setup.pkg` |

Install prerequisites and instructions: [CPU packages](https://github.com/{REPOSITORY}/blob/{revision}/docs/cpu_release_packages.md).
Archives, SHA-256 checksums, native qualification receipts and candidate metadata are attached.
These are unsigned development installers, not GA or an expansion of the runtime production allowlist.
Windows/macOS/ARM64 CPU experiments require the documented explicit opt-in.
This release makes no measured energy-saving or certification claim.

[Build and benchmark evidence](https://github.com/{REPOSITORY}/actions/runs/{os.environ['GITHUB_RUN_ID']}).
""")
    created = False
    tagged = False
    try:
        gh("api", f"repos/{REPOSITORY}/git/refs", "--method", "POST",
           "-f", f"ref=refs/tags/{tag}", "-f", f"sha={revision}")
        tagged = True
        gh("release", "create", tag, *assets, "--repo", REPOSITORY, "--target", revision,
           "--title", f"ShortHand development {tag}", "--notes-file", notes,
           "--draft", "--verify-tag", "--prerelease", "--latest=false")
        created = True
        actual = json.loads(gh("api", f"repos/{REPOSITORY}/git/ref/tags/{tag}").stdout)
        if actual.get("object", {}).get("sha") != revision:
            raise ValueError("release tag does not identify the tested revision")
        with tempfile.TemporaryDirectory(prefix="shorthand release verification ") as temporary:
            download = pathlib.Path(temporary)
            gh("release", "download", tag, "--repo", REPOSITORY, "--dir", download)
            if {p.name for p in download.iterdir()} != set(hashes):
                raise ValueError("uploaded release asset set mismatch")
            for name, expected in hashes.items():
                if digest(download / name) != expected:
                    raise ValueError(f"uploaded release digest mismatch: {name}")
        gh("release", "edit", tag, "--repo", REPOSITORY, "--draft=false", "--prerelease", "--latest=false")
    except BaseException:
        if created:
            # Only this invocation's draft is eligible for rollback.
            gh("release", "delete", tag, "--repo", REPOSITORY, "--yes", "--cleanup-tag")
        elif tagged:
            gh("api", f"repos/{REPOSITORY}/git/refs/tags/{tag}", "--method", "DELETE")
        raise
    print(f"PUBLISHED_BUILD_RELEASE https://github.com/{REPOSITORY}/releases/tag/{tag}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=pathlib.Path)
    args = parser.parse_args()
    tag, revision = release_identity(os.environ)
    publish(args.directory.resolve(strict=True), tag, revision)


if __name__ == "__main__":
    main()
