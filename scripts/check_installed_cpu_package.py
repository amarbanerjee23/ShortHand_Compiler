#!/usr/bin/env python3
"""Execute a real ONNX model from the final archive with build/SDK paths hidden."""
import argparse
import base64
import hashlib
import json
import os
import pathlib
import platform
import subprocess
import tempfile
import uuid

ROOT = pathlib.Path(__file__).resolve().parents[1]


def run(command, *, cwd, env, expect_failure=False):
    result = subprocess.run([str(x) for x in command], cwd=cwd, env=env,
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=300)
    print(result.stdout, end="", flush=True)
    if expect_failure == (result.returncode == 0):
        raise RuntimeError(f"unexpected exit {result.returncode}: {command[0]}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=pathlib.Path)
    parser.add_argument("platform", choices=["linux-x64", "linux-arm64", "macos-arm64", "windows-x64"])
    parser.add_argument("--sdk", type=pathlib.Path, required=True)
    parser.add_argument("--build", type=pathlib.Path, required=True)
    parser.add_argument("--report", type=pathlib.Path, required=True)
    args = parser.parse_args()
    native = (platform.system(), platform.machine().lower())
    expected = {"linux-x64": ("Linux", {"x86_64", "amd64"}),
                "linux-arm64": ("Linux", {"aarch64", "arm64"}),
                "macos-arm64": ("Darwin", {"arm64"}),
                "windows-x64": ("Windows", {"amd64", "x86_64"})}[args.platform]
    if native[0] != expected[0] or native[1] not in expected[1]:
        raise RuntimeError(f"native qualification mismatch: {native} for {args.platform}")
    archive, sdk, build = (p.resolve(strict=True) for p in (args.archive, args.sdk, args.build))
    report = args.report.resolve()
    report.parent.mkdir(parents=True, exist_ok=True)
    report.unlink(missing_ok=True)  # A failed attempt must never leave a stale pass.
    if sdk in archive.parents or build in archive.parents or sdk in build.parents or build in sdk.parents:
        raise RuntimeError("archive, build and SDK must have independent locations")
    hidden = []
    try:
        for path in (sdk, build):
            destination = path.with_name(path.name + ".qualification-hidden-" + uuid.uuid4().hex)
            path.rename(destination)
            hidden.append((path, destination))
        with tempfile.TemporaryDirectory(prefix="shorthand installed cpu ") as temporary:
            work = pathlib.Path(temporary)
            prefix = work / "relocated SDK"
            prefix.mkdir()
            env = os.environ.copy()
            for name in ("PATH", "LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "LIBRARY_PATH", "CMAKE_PREFIX_PATH"):
                if name in env:
                    env[name] = os.pathsep.join(item for item in env[name].split(os.pathsep)
                                               if item and str(sdk) not in item and str(build) not in item
                                               and not any(pathlib.Path(item).glob("*onnxruntime*")))
            for name in ("LD_PRELOAD", "DYLD_INSERT_LIBRARIES", "DYLD_FALLBACK_LIBRARY_PATH"):
                env.pop(name, None)
            env.pop("ONNXRUNTIME_ROOT", None)
            env.pop("SHORTHAND_RUNTIME_LIB", None)
            env.pop("SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE", None)
            # These paths must not select an unrelated installed ShortHand SDK.
            env.pop("ShortHand_DIR", None)
            run(["cmake", "-E", "tar", "xf", archive], cwd=prefix, env=env)
            suffix = ".exe" if native[0] == "Windows" else ""
            compiler = prefix / "bin" / ("short_hand" + suffix)
            green = prefix / "bin" / ("green_ai_tool" + suffix)
            if not compiler.is_file() or not green.is_file():
                raise RuntimeError("both installed CLI tools are required")
            env["PATH"] = str(prefix / "bin") + os.pathsep + env.get("PATH", "")
            for metadata in prefix.rglob("*.cmake"):
                value = metadata.read_text()
                if str(sdk).replace("\\", "/") in value or str(build).replace("\\", "/") in value:
                    raise RuntimeError(f"installed CMake metadata retains a build path: {metadata.name}")
            output = run([compiler, ROOT / "tests/semantic/differential/core_control.short", "run"], cwd=work, env=env)
            if output.strip() != (ROOT / "tests/semantic/differential/core_control.expected").read_text().strip():
                raise RuntimeError("installed interpreter result mismatch")
            # Exercise the second installed CLI too; this is its documented usage error.
            output = run([green], cwd=work, env=env, expect_failure=True)
            if "usage: green_ai_tool" not in output:
                raise RuntimeError("installed Green AI CLI did not execute")
            models = work / "model files"
            models.mkdir()
            model = base64.b64decode((ROOT / "tests/fixtures/onnx/identity_float32_v13.onnx.b64").read_bytes())
            (models / "identity.onnx").write_bytes(model)
            model_path = models / "model with spaces.onnx"
            model_path.write_bytes(model)
            fixture = ROOT / "tests/packaging/cpu_consumer"
            consumer = work / "consumer"
            configure = ["cmake", "-S", fixture, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
                         f"-DCMAKE_PREFIX_PATH={prefix}", f"-DPACKAGE_BIN={prefix / 'bin'}",
                         f"-DMODEL_FILE={model_path}", f"-DMODEL_DIRECTORY={models}"]
            run([*configure, "-B", consumer], cwd=work, env=env)
            run(["cmake", "--build", consumer, "--parallel", "2"], cwd=work, env=env)
            qualification_mode = "existing_production_scope"
            if args.platform != "linux-x64":
                # Exercise the current fail-closed production policy first.
                # Explicit native experiments collect evidence for future scope
                # promotion; they must never claim production qualification.
                for kind in ("static", "shared"):
                    run([consumer / ("cpu_" + kind + suffix), model_path, "--expect-unqualified"], cwd=work, env=env)
                env["SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE"] = "1"
                qualification_mode = "experimental_native_candidate"
            run(["ctest", "--test-dir", consumer, "--output-on-failure", "-V"], cwd=work, env=env)
            # The interpreter uses the non-prepared ORT path; compiled consumers
            # exercise the C ABI and prepared cache. Both must execute on Windows.
            output = run([compiler, fixture / "interpreter_cpu.short", "run"], cwd=models, env=env)
            if "AI inference output: 42" not in output or "fallback" in output or "not_executed" in output:
                raise RuntimeError("interpreter must execute the real model and return 42")
            runtime = next(prefix.rglob("onnxruntime.dll"), None) if suffix else next(
                prefix.rglob("libonnxruntime.dylib" if native[0] == "Darwin" else "libonnxruntime.so"), None)
            if runtime is None:
                raise RuntimeError("bundled ONNX runtime missing")
            runtime.resolve().unlink()
            output = run([*configure, "-B", work / "missing-runtime"], cwd=work, env=env, expect_failure=True)
            if "ShortHand_ONNXRUNTIME_LIBRARY" not in output:
                raise RuntimeError("missing-runtime negative failed for an unrelated reason")
            receipt = {"schema": "shorthand.release.cpu_package.v1", "status": "pass",
                       "platform": args.platform, "native_os": native[0], "architecture": native[1],
                       "qualification_mode": qualification_mode,
                       "archive_sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
                       "source_revision": subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip(),
                       "source_dirty": bool(subprocess.check_output(["git", "-C", str(ROOT), "status", "--porcelain", "--untracked-files=no"], text=True).strip()),
                       "onnxruntime_version": (prefix / "share/shorthand/licenses/onnxruntime/VERSION_NUMBER").read_text().strip(),
                       "compiler_sha256": hashlib.sha256(compiler.read_bytes()).hexdigest(),
                       "checks": ["relocated_archive", "build_and_sdk_hidden", "installed_clis", "static_cpu_numerics",
                                  "shared_cpu_numerics", "compiled_core_source", "interpreter_cpu", "nonfinite_rollback",
                                  "missing_runtime_rejected", "production_scope_guard"], "production_claim": False}
            report.write_text(json.dumps(receipt, indent=2) + "\n")
            print(f"PASS native installed CPU package {args.platform}")
    finally:
        for original, destination in reversed(hidden):
            destination.rename(original)


if __name__ == "__main__":
    main()
