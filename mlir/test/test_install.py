"""Qualify a relocated install with an independent CMake consumer."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

source, build = (Path(p).resolve() for p in sys.argv[1:3])
mlir_dir, compiler, sanitizers = sys.argv[3:]


def run(*args):
    subprocess.run(args, check=True, timeout=180)


with tempfile.TemporaryDirectory(prefix="shorthand-mlir-install-") as tmp:
    root = Path(tmp)
    prefix = root / "initial"
    moved = root / "relocated"
    run("cmake", "--install", str(build), "--prefix", str(prefix))
    prefix.rename(moved)
    consumer = root / "consumer"
    shutil.copytree(source / "test/consumer", consumer)
    consumer_build = root / "consumer-build"
    packages = list(moved.glob("lib*/cmake/ShortHandMLIR/ShortHandMLIRConfig.cmake"))
    if len(packages) != 1:
        raise AssertionError("installed package config missing or ambiguous")
    run("cmake", "-S", str(consumer), "-B", str(consumer_build), "-G", "Ninja",
        f"-DShortHandMLIR_DIR={packages[0].parent}", f"-DMLIR_DIR={mlir_dir}",
        "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_CXX_COMPILER={compiler}", f"-DSHORTHAND_MLIR_SANITIZERS={sanitizers}")
    run("cmake", "--build", str(consumer_build), "--parallel", "2")
    # Include paths must come from the relocated package, never the checkout or
    # generated build headers. Verify both exports and actual compile commands.
    metadata = (consumer_build / "compile_commands.json").read_text()
    for cmake in packages[0].parent.glob("*.cmake"):
        metadata += cmake.read_text()
    if str(source) in metadata or str(build) in metadata or str(prefix) in metadata:
        raise AssertionError("installed consumer depends on source/build/old install paths")
    example = moved / "share/shorthand/mlir/examples/ai_greenai_pipeline.mlir"
    run(str(consumer_build / "consumer"), str(example))
    lowering = consumer_build / "lowering_consumer"
    run(str(lowering), "verify")
    for level in ("O0", "O2"):
        ir = root / f"api-{level}.ll"
        run(str(lowering), "api", str(ir), level)
        native = root / f"api-{level}"
        clang = Path(mlir_dir).resolve().parents[2] / "bin/clang++"
        run(str(clang), str(ir), f"-{level}", "-o", str(native))
        result = subprocess.run([str(native)], check=True, capture_output=True, text=True, timeout=20)
        if result.stdout != "42\n": raise AssertionError("installed composite call/return changed")
    driver = moved / "bin/shorthand-opt"
    text = root / "roundtrip.mlir"
    bytecode = root / "roundtrip.mlirbc"
    run(str(driver), str(example), "--emit-bytecode", "-o", str(bytecode))
    run(str(driver), str(bytecode), "-o", str(text))
    # Installed TableGen definitions must be consumable without the source tree.
    tblgen = Path(mlir_dir).resolve().parents[2] / "bin/mlir-tblgen"
    includes = Path(mlir_dir).resolve().parents[2] / "include"
    run(str(tblgen), str(moved / "include/ShortHand/IR/ShortHandOps.td"),
        "-I", str(moved / "include"), "-I", str(includes), "-gen-op-decls",
        "-o", str(root / "consumer-ops.h.inc"))
print("PASS relocated MLIR install, four self-contained headers, API consumer, bytecode and installed TableGen")
