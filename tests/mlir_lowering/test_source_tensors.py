"""Real source tensor values across interpreter, LLVM and MLIR execution."""
from pathlib import Path
import base64
import os
import subprocess
import sys
import tempfile

root, build, sdk = (Path(x).resolve() for x in sys.argv[1:4])
compiler = build / "short_hand"
clang = os.environ.get("SHORTHAND_LLVM_CLANG", "clang++")
env = dict(os.environ)
env["LD_LIBRARY_PATH"] = str(sdk / "lib") + ":" + env.get("LD_LIBRARY_PATH", "")
env["ORT_DISABLE_TELEMETRY"] = "1"
flags = (["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
         if os.environ.get("SHORTHAND_MLIR_SANITIZERS") == "ON" else [])
checks = 0


def run(*args, success=True):
    global checks
    result = subprocess.run(list(map(str, args)), cwd=work, env=env,
                            text=True, capture_output=True, timeout=90)
    checks += 1
    assert result.returncode >= 0 and (result.returncode == 0) == success, (
        args, result.returncode, result.stdout, result.stderr)
    assert not any(x in result.stderr for x in (
        "AddressSanitizer", "LeakSanitizer", "runtime error:",
        "PLEASE submit a bug report", "flex scanner jammed")), result.stderr
    return result


def source(name, text):
    path = work / (name + ".short")
    # Preserve the existing grammar's ordinary declaration section.
    path.write_text("int tensor_test_guard;\n" + text)
    return path


def execute(name, text, expected="", error=None):
    path = source(name, text)
    result = run(compiler, path, "run", success=error is None)
    stdout = "".join(line for line in result.stdout.splitlines(keepends=True)
                     if not line.startswith("Registered model "))
    assert error in result.stderr if error else stdout == expected, result
    for mode, suffix in (("compile-bc", ".bc"), ("compile-mlir", ".ll")):
        ir = work / (name + suffix)
        run(compiler, path, mode, "--output", ir)
        for level in ("-O0", "-O2"):
            executable = work / (name + suffix + level)
            run(clang, ir, level, *flags, build / "libshorthand_runtime.a",
                "-L", sdk / "lib", "-lonnxruntime", "-pthread", "-o", executable)
            result = run(executable, success=error is None)
            assert error in result.stderr if error else result.stdout == expected, result


def reject(name, text, diagnostic="SHD", semantic=False):
    path = source(name, text)
    for mode in ("run", "compile-bc", "compile-mlir"):
        result = run(compiler, path, mode, success=False)
        assert diagnostic in result.stderr, result
        if semantic:
            assert "SHD2001" not in result.stderr, result


def model(path="identity.onnx", shape="1", precision="float"):
    return f'''model identity {{
      format onnx; path "{path}"; task "identity"; precision {precision};
      input_shape "{shape}"; output_shape "{shape}";
      backend_preference onnxruntime_cpu; compact false;
      quality_guardrail accuracy >= 1;
    }};\n'''


def large_identity():
    """Independent minimal protobuf fixture: FP32 Identity with 65,536 values."""
    def varint(n):
        out = bytearray()
        while n >= 128:
            out.append((n & 127) | 128)
            n >>= 7
        return bytes(out) + bytes([n])
    def integer(field, n):
        return varint(field << 3) + varint(n)
    def message(field, value):
        if isinstance(value, str):
            value = value.encode()
        return varint((field << 3) | 2) + varint(len(value)) + value
    tensor_type = integer(1, 1) + message(2, message(1, integer(1, 65536)))
    def info(name):
        return message(1, name) + message(2, message(1, tensor_type))
    node = message(1, "X") + message(2, "Y") + message(4, "Identity")
    graph = (message(1, node) + message(2, "large_identity")
             + message(11, info("X")) + message(12, info("Y")))
    return integer(1, 8) + message(7, graph) + message(8, integer(2, 13))


with tempfile.TemporaryDirectory(prefix="shorthand-source-tensors-") as tmp:
    work = Path(tmp)
    execute("values", '''int i; float scalar;
      tensor data fp32 "2,2" = [1.25,-2,3.5,4]; tensor zero float "2";
      scalar=16777217.0; data[0]=scalar; data[1]=data[2]+0.25;
      i=3; data[i]=data[1];
      print scalar,data[0],data[1],data[2],data[3],zero[0],zero[1];
    ''', "16777217 16777216 3.75 3.5 3.75 0 0\n")
    execute("large_integer", '''tensor data fp32 " +2 " = [2147483648,-2147483648];
      print data[0],data[1];''', "2147483648 -2147483648\n")
    execute("reset", '''int i;
      loop i<2 {tensor data float "1"=[2.5]; print data[0];
        data[0]=9.5; i=i+1;}
      {float data[1]; data[0]=8.5; print data[0];}
    ''', "2.5\n2.5\n8.5\n")
    execute("recursive", '''float value;
      def float recurse(int n;) {
        tensor shared float "1"=[2.5];
        if n>0 {shared[0]=recurse(n-1)+1.0;}
        return shared[0];
      };
      value=recurse(3); print value,shared[0];
    ''', "5.5 5.5\n")
    execute("order", '''def float rhs() {print "rhs"; return 7.25;};
      def int index() {print "index"; return 0;};
      tensor data fp32 "1"; data[index()]=rhs(); print data[0];
    ''', "rhs\nindex\n7.25\n")
    execute("negative_index", 'tensor data float "2"; print data[-1];', error="SHD7002")
    execute("high_index", 'tensor data float "2"; data[2]=1.0;', error="SHD7002")
    huge = "10000000000000000000000000000000000000000"
    execute("overflow_store", f'tensor data fp32 "1"; data[0]={huge}.0;', error="SHD7004")
    execute("not_ready", 'if false {tensor data fp32 "1";} print data[0];', error="SHD7004")
    execute("limit", 'tensor data fp32 "65536"; data[65535]=42.5; print data[0],data[65535];', "0 42.5\n")
    for name, text in {
        "wrong_count": 'tensor data fp32 "2"=[1];',
        "empty": 'tensor data fp32 "1"=[];',
        "integer_type": 'tensor data int8 "1"=[1];',
        "dynamic": 'tensor data fp32 "?"=[1];',
        "shape_overflow": 'tensor data fp32 "9223372036854775807,2"=[1];',
        "too_large": 'tensor data fp32 "65537"=[1];',
        "float_overflow": f'tensor data fp32 "1"=[{huge}.0];',
        "integer_overflow": f'tensor data fp32 "1"=[{huge}];',
        "float_index": 'tensor data fp32 "1"; print data[0.0];',
        "integer_target": 'int value; tensor data fp32 "1"; value=data[0];',
        "read_tensor": 'tensor data fp32 "1"; read data[0];',
    }.items():
        reject(name, text, semantic=True)
    reject("leading_comma", 'tensor data fp32 "1"=[,1];')
    reject("trailing_comma", 'tensor data fp32 "1"=[1,];')
    reject("too_many_literals", 'tensor data fp32 "1"=[' + ','.join(['1'] * 65537) + '];')

    artifact = work / "identity.onnx"
    artifact.write_bytes(base64.b64decode(
        (root / "tests/fixtures/onnx/identity_float32_v13.onnx.b64").read_text()))
    inference = model() + '''tensor input fp32 "1"=[42.5]; tensor output float "1";
      infer identity(input) -> output; print output[0];
      output[0]=output[0]+1.0; infer identity(output) -> output; print output[0];'''
    execute("inference", inference, "42.5\n43.5\n")
    execute("input_not_ready", model() + '''if false {tensor input fp32 "1"=[1];}
      tensor output fp32 "1"; infer identity(input) -> output;''', error="SHD7004")
    (work / "large.onnx").write_bytes(large_identity())
    execute("large_inference", model("large.onnx", "65536") + '''
      tensor input fp32 "65536"; tensor output fp32 "65536";
      input[0]=3.5; input[65535]=7.25; infer identity(input) -> output;
      print output[0],output[65535];''', "3.5 7.25\n")
    for name, declaration in (("wrong_precision", model(precision="fp16")),
                              ("dynamic_model", model(shape="?"))):
        reject(name, declaration + '''tensor input fp32 "1"=[1]; tensor output fp32 "1";
          infer identity(input) -> output;''', "matching static FP32", semantic=True)
    artifact.write_bytes(b"not an ONNX model")
    execute("malformed_model", inference, error="SHD7030")
    artifact.unlink()
    execute("missing_model", inference, error="SHD7030")

print(f"PASS source tensor values, FP32 rounding, bounds, lifetime, real inference and failures ({checks} commands)")
