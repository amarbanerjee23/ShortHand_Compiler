"""Detect stale generated files and broken transitive TableGen dependencies."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

source, build = (Path(p).resolve() for p in sys.argv[1:3])
mlir_dir = sys.argv[3]


def run(*args):
    subprocess.run(args, check=True, timeout=120, stdout=subprocess.DEVNULL)


with tempfile.TemporaryDirectory(prefix="shorthand-mlir-generation-") as tmp:
    root = Path(tmp)
    copied = root / "source"
    shutil.copytree(source, copied)
    generated = root / "build"
    run("cmake", "-S", str(copied), "-B", str(generated), "-G", "Ninja",
        f"-DMLIR_DIR={mlir_dir}", "-DSHORTHAND_MLIR_TESTING=OFF")
    run("cmake", "--build", str(generated), "--target", "ShortHandMLIRIncGen")
    relative = Path("include/ShortHand/IR")
    outputs = sorted((build / relative).glob("*.inc"))
    if len(outputs) != 8:
        raise AssertionError("expected all eight generated dialect/type/attribute/op files")
    for output in outputs:
        if output.read_bytes() != (generated / relative / output.name).read_bytes():
            raise AssertionError(f"stale generation: {output.name}")
    for kind in ("Dialect", "Types", "Attributes", "Ops"):
        td = copied / relative / f"ShortHand{kind}.td"
        original = td.read_text()
        marker = f"static constexpr int freshness{kind} = 92;"
        td.write_text(original.replace("  let summary", f"  let extraClassDeclaration = [{{{marker}}}];\n  let summary", 1))
        run("cmake", "--build", str(generated), "--target", "ShortHandMLIRIncGen")
        header = generated / relative / f"ShortHand{kind}.h.inc"
        if marker not in header.read_text():
            raise AssertionError(f"{kind} mutation did not regenerate declarations")
        td.write_text(original)
        run("cmake", "--build", str(generated), "--target", "ShortHandMLIRIncGen")
        if marker in header.read_text():
            raise AssertionError(f"{kind} generated file stayed stale after restoration")
    # Ops include both type and attribute definitions. A missing included source
    # must fail the incremental build rather than silently retaining old output.
    (copied / relative / "ShortHandAttributes.td").unlink()
    result = subprocess.run(["cmake", "--build", str(generated), "--target", "ShortHandMLIRIncGen"],
                            capture_output=True, text=True, timeout=120)
    if result.returncode == 0 or "ShortHandAttributes.td" not in result.stdout + result.stderr:
        raise AssertionError("missing TableGen dependency was accepted")
print("PASS MLIR generation freshness, four incremental mutations and missing-dependency rejection")
