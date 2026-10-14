#!/usr/bin/env python3
"""Build, install, qualify and uninstall the exact release archive on its native OS.

Run only on disposable CI machines with the documented toolchain prerequisites.
No pass receipt is written until the complete installation lifecycle succeeds.
"""
import argparse
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

from verify_release_candidates import verify_receipt

ROOT = pathlib.Path(__file__).resolve().parents[1]
PACKAGE_ID = "org.shorthand.compiler"


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def run(command, **kwargs):
    subprocess.run(command, check=True, timeout=1200, **kwargs)


def nsis_quote(value):
    return str(value).replace("$", "$$").replace('"', '$\\"')


def nsis_path(value):
    # MSYS2 Python can emit C:/ paths. NSIS validates roots and strips forward
    # slashes at runtime, so both script paths and CLI overrides need backslashes.
    # PureWindowsPath also follows MSYS2's patched ntpath.sep in Python 3.14.
    return nsis_quote(str(pathlib.PureWindowsPath(value)).replace("/", "\\"))


def nsis_command(executable, prefix, *, uninstall=False):
    executable, prefix = map(pathlib.PureWindowsPath, (executable, prefix))
    if not executable.is_absolute() or not prefix.is_absolute():
        raise ValueError("NSIS requires absolute Windows paths")
    if '"' in str(executable) or '"' in str(prefix):
        raise ValueError("invalid quote in NSIS path")
    executable, prefix = (str(p).replace("/", "\\") for p in (executable, prefix))
    # NSIS consumes the entire unquoted remainder for /D= and _?=, including spaces.
    override = "_?=" if uninstall else "/D="
    return f'"{executable}" /S {override}{prefix}'


def windows_script(payload, installer, version):
    # Delete only packaged files; retain any files subsequently created by users.
    files = sorted(p.relative_to(payload) for p in payload.rglob("*") if p.is_file())
    dirs = sorted((p.relative_to(payload) for p in payload.rglob("*") if p.is_dir()),
                  key=lambda p: len(p.parts), reverse=True)
    deletion = "\n".join(f'  Delete "$INSTDIR\\{nsis_path(p)}"' for p in files)
    deletion += "\n" + "\n".join(f'  RMDir "$INSTDIR\\{nsis_path(p)}"' for p in dirs)
    return f'''Unicode true
Name "ShortHand {version}"
OutFile "{nsis_path(installer)}"
InstallDir "$LOCALAPPDATA\\Programs\\ShortHand"
RequestExecutionLevel user
SetCompressor /SOLID lzma
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles
Section "ShortHand"
  IfFileExists "$INSTDIR\\uninstall.exe" 0 +3
    MessageBox MB_OK "Uninstall the previous ShortHand installation first."
    Abort
  SetOutPath "$INSTDIR"
  File /r "{nsis_path(payload)}\\*"
  WriteUninstaller "$INSTDIR\\uninstall.exe"
  WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ShortHand" "DisplayName" "ShortHand {version}"
  WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ShortHand" "UninstallString" '$\\"$INSTDIR\\uninstall.exe$\\"'
  WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ShortHand" "InstallLocation" "$INSTDIR"
SectionEnd
Section "Uninstall"
{deletion}
  Delete "$INSTDIR\\uninstall.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ShortHand"
SectionEnd
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=pathlib.Path)
    parser.add_argument("platform", choices=["linux-x64", "linux-arm64", "macos-arm64", "windows-x64"])
    parser.add_argument("--sdk", type=pathlib.Path, required=True)
    parser.add_argument("--build", type=pathlib.Path, required=True)
    args = parser.parse_args()
    bundle = args.bundle.resolve(strict=True)
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    verify_receipt(bundle, args.platform, revision)
    manifest = dict(line.split("=", 1) for line in next(bundle.glob("*.manifest")).read_text().splitlines())
    version = manifest["version"].removeprefix("v")
    if not re.fullmatch(r"\d+\.\d+\.\d+(?:-rc\.\d+)?", version):
        raise ValueError("unsupported installer version")
    archive = bundle / manifest["artifact"]
    extension = ".exe" if args.platform.startswith("windows") else ".pkg" if args.platform.startswith("macos") else ".deb"
    installer = bundle / (archive.stem + "-setup" + extension)
    receipt = bundle / (archive.stem + ".installer-qualification.json")
    receipt.unlink(missing_ok=True)
    installer.unlink(missing_ok=True)
    with tempfile.TemporaryDirectory(prefix="shorthand installer ") as temporary:
        work = pathlib.Path(temporary)
        payload = work / "payload"
        payload.mkdir()
        run(["cmake", "-E", "tar", "xf", str(archive)], cwd=payload)
        # These snapshots bind the installer to every file in the tested archive.
        contents = {p.relative_to(payload): digest(p) for p in payload.rglob("*") if p.is_file()}
        if extension == ".exe":
            prefix = work / "installed with spaces"
            script = work / "installer.nsi"
            script.write_text(windows_script(payload, installer, version), encoding="utf-8")
            run(["makensis", str(script)])
            install = nsis_command(installer, prefix)
            print(f"NSIS install command: {install}", flush=True)
        elif extension == ".deb":
            prefix = pathlib.Path("/opt/shorthand")
            package = work / "deb"
            shutil.copytree(payload, package / "opt/shorthand", symlinks=True)
            (package / "DEBIAN").mkdir()
            arch = "amd64" if args.platform == "linux-x64" else "arm64"
            (package / "DEBIAN/control").write_text(
                f"Package: shorthand\nVersion: {version.replace('-rc.', '~rc.')}\n"
                f"Architecture: {arch}\nMaintainer: ShortHand maintainers <noreply@github.com>\n"
                "Section: devel\nPriority: optional\n"
                "Depends: libc6 (>= 2.38), libstdc++6, libgcc-s1, libssl3t64, llvm-18\n"
                "Recommends: clang-18, cmake, ninja-build\n"
                "Description: ShortHand compiler and ONNX CPU runtime\n")
            run(["dpkg-deb", "--root-owner-group", "--build", str(package), str(installer)])
            install = ["sudo", "dpkg", "--install", str(installer)]
        else:
            prefix = pathlib.Path("/opt/shorthand")
            run(["pkgbuild", "--root", str(payload), "--identifier", PACKAGE_ID,
                 "--version", version.replace("-rc.", "."), "--install-location", str(prefix), str(installer)])
            install = ["sudo", "installer", "-pkg", str(installer), "-target", "/"]
        if prefix.exists():
            raise RuntimeError(f"refusing to overwrite existing installation: {prefix}")
        attempted = False
        try:
            attempted = True
            run(install)
            for relative, expected in contents.items():
                if digest(prefix / relative) != expected:
                    raise RuntimeError(f"installer payload mismatch: {relative}")
            # Linux/macOS package files belong to root. Qualification deliberately
            # removes ONNX for its negative control, so grant the CI user ownership
            # of this dedicated, newly installed prefix before that destructive test.
            if extension != ".exe":
                run(["sudo", "chown", "-R", str(os.getuid()), str(prefix)])
            report = work / "installed-cpu.json"
            run([sys.executable, str(ROOT / "scripts/check_installed_cpu_package.py"),
                 str(archive), args.platform, "--sdk", str(args.sdk.resolve()),
                 "--build", str(args.build.resolve()), "--report", str(report),
                 "--installed-prefix", str(prefix)])
            installed_report = json.loads(report.read_text())
            if installed_report.get("status") != "pass" or installed_report.get("source_revision") != revision:
                raise RuntimeError("native installer CPU qualification failed")
        finally:
            if attempted:
                if extension == ".exe" and (prefix / "uninstall.exe").exists():
                    # Avoid NSIS's asynchronous copy-to-temp uninstaller behavior.
                    uninstaller = work / "uninstall.exe"
                    shutil.copy2(prefix / "uninstall.exe", uninstaller)
                    run(nsis_command(uninstaller, prefix, uninstall=True))
                elif extension == ".deb":
                    run(["sudo", "dpkg", "--remove", "shorthand"])
                elif extension == ".pkg":
                    # macOS has no package uninstall command. Remove only the
                    # exact newly installed payload and forget this receipt.
                    for relative in contents:
                        (prefix / relative).unlink(missing_ok=True)
                    for directory in sorted(prefix.rglob("*"), key=lambda p: len(p.parts), reverse=True):
                        if directory.is_dir():
                            directory.rmdir()
                    run(["sudo", "rmdir", str(prefix)])
                    run(["sudo", "pkgutil", "--forget", PACKAGE_ID])
        if any((prefix / relative).exists() for relative in contents):
            raise RuntimeError("uninstaller left packaged files behind")
        result = {"schema": "shorthand.release.installer.v1", "status": "pass",
                  "platform": args.platform, "source_revision": revision,
                  "archive_sha256": digest(archive), "installer": installer.name,
                  "installer_sha256": digest(installer), "production_claim": False,
                  "checks": ["native_install", "exact_archive_payload", "installed_cpu_suite", "native_uninstall"]}
        receipt.write_text(json.dumps(result, indent=2) + "\n")
        installer.with_suffix(installer.suffix + ".SHA256SUMS").write_text(f"{digest(installer)}  {installer.name}\n")
        print(f"PASS native installer lifecycle {args.platform}: {installer.name}")


if __name__ == "__main__":
    main()
