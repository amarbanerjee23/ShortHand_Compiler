"""Native bounded importer versus independent pinned Python/ONNX inspection."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

import numpy as np
import onnx
from onnx import helper, numpy_helper, TensorProto

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "experiments/aot"))
from compile_model import inspect_model


def affine(m=3, k=5, n=7, encoding="raw", reverse_bias=False):
    rng = np.random.default_rng(47)
    w = rng.uniform(-2, 2, (k, n)).astype(np.float32)
    b = rng.uniform(-1, 1, n).astype(np.float32)
    w.flat[0] = -0.0
    if w.size > 1:
        w.flat[1] = np.nextafter(np.float32(0), np.float32(1))
    if encoding == "raw":
        initializers = [numpy_helper.from_array(w, "W"), numpy_helper.from_array(b, "B")]
    else:
        initializers = [helper.make_tensor("W", TensorProto.FLOAT, w.shape, w.ravel().tolist()),
                        helper.make_tensor("B", TensorProto.FLOAT, b.shape, b.tolist())]
    graph = helper.make_graph([helper.make_node("MatMul", ["X", "W"], ["M"]),
                               helper.make_node("Add", ["B", "M"] if reverse_bias else ["M", "B"], ["Y"])],
                              "native_affine", [helper.make_tensor_value_info("X", TensorProto.FLOAT, [m, k])],
                              [helper.make_tensor_value_info("Y", TensorProto.FLOAT, [m, n])], initializers,
                              value_info=[helper.make_tensor_value_info("M", TensorProto.FLOAT, [m, n])])
    model = helper.make_model(graph, ir_version=8, opset_imports=[helper.make_opsetid("", 13)])
    return model


def run(argv, **kwargs):
    result = subprocess.run(list(map(str, argv)), text=True, capture_output=True, timeout=90, **kwargs)
    if result.returncode or any(s in result.stderr for s in ("AddressSanitizer", "LeakSanitizer", "runtime error:")):
        raise RuntimeError((argv, result.returncode, result.stdout, result.stderr))
    return result.stdout


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--sanitizers", action="store_true")
    args = parser.parse_args()
    checks = 0
    with tempfile.TemporaryDirectory(prefix="shorthand-native-aot-") as tmp:
        work = Path(tmp); binary = work / "inspect"
        src = ROOT / "Compiler_new_ws/Short_Hand/src"
        flags = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if args.sanitizers else []
        run([args.cxx, "-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", *flags,
             "-I" + str(src), ROOT / "tests/ai_application/native_aot_import_probe.cpp",
             src / "aot/StaticAffineModel.cpp", src / "module/Sha256.cpp", "-o", binary])

        def inspect(data, disposition):
            nonlocal checks
            if isinstance(data, onnx.ModelProto): data = data.SerializeToString()
            path = work / "input.onnx"; path.write_bytes(data)
            result = json.loads(run([binary, path])); checks += 1
            assert result["disposition"] == disposition, result
            if disposition == 2:
                assert result["owned_bytes"] == 0 and not result["sha256"] and result["shape"] == [0, 0, 0], result
            else:
                assert result["owned_bytes"] == len(data) and result["sha256"] == hashlib.sha256(data).hexdigest(), result
            if disposition == 0:
                shape, arrays = inspect_model(data)
                assert result["shape"] == list(shape), result
                for key, array in zip(("weights_sha256", "bias_sha256"), arrays):
                    assert result[key] == hashlib.sha256(array.astype("<f4").tobytes()).hexdigest(), result
            return result

        for encoding in ("raw", "float_data"):
            for reverse in (False, True):
                inspect(affine(encoding=encoding, reverse_bias=reverse), 0)
        for dims in ((1, 1, 1), (1, 64, 10), (16, 64, 10), (32, 64, 10), (64, 1024, 1024)):
            inspect(affine(*dims), 0)
        model = affine()
        model.ir_version = 10
        model.producer_name = "ShortHand native importer test"
        model.doc_string = "UTF-8 metadata: café"
        model.metadata_props.add(key="purpose", value="test")
        model.graph.node[0].metadata_props.add(key="note", value="not an attribute")
        inspect(model, 0)
        # Full byte boundary, including benign metadata; then one byte over it.
        padded = affine(); padded.doc_string = "x" * (16 * 1024 * 1024 - len(padded.SerializeToString()) - 5)
        data = padded.SerializeToString()
        assert len(data) == 16 * 1024 * 1024
        inspect(data, 0); inspect(data + b"\0", 2)

        base = affine()
        def reject(mutator):
            candidate = copy.deepcopy(base); mutator(candidate); inspect(candidate, 2)
        for version in (0, 7, 11): reject(lambda m, v=version: setattr(m, "ir_version", v))
        reject(lambda m: setattr(m.opset_import[0], "version", 12))
        reject(lambda m: setattr(m.opset_import[0], "domain", "custom"))
        reject(lambda m: m.opset_import.add(domain="", version=13))
        reject(lambda m: m.ClearField("graph"))
        reject(lambda m: m.graph.input.add().CopyFrom(m.graph.input[0]))
        reject(lambda m: m.graph.ClearField("output"))
        reject(lambda m: setattr(m.graph.input[0].type.tensor_type, "elem_type", TensorProto.DOUBLE))
        for dimension in (0, -1, 1025):
            reject(lambda m, d=dimension: setattr(m.graph.input[0].type.tensor_type.shape.dim[0], "dim_value", d))
        reject(lambda m: setattr(m.graph.input[0].type.tensor_type.shape.dim[0], "dim_param", "batch"))
        reject(lambda m: m.graph.input[0].type.tensor_type.shape.dim.add(dim_value=1))
        reject(lambda m: setattr(m.graph.output[0].type.tensor_type.shape.dim[0], "dim_value", 4))
        reject(lambda m: setattr(m.graph.value_info[0].type.tensor_type.shape.dim[1], "dim_value", 8))
        reject(lambda m: m.graph.node[0].input.__setitem__(0, "missing"))
        reject(lambda m: m.graph.node[0].input.__setitem__(0, "Y"))
        reject(lambda m: m.graph.node[1].output.__setitem__(0, "X"))
        reject(lambda m: setattr(m.graph.output[0], "name", "missing"))
        reject(lambda m: setattr(m.graph.initializer[1], "name", "W"))
        reject(lambda m: m.graph.initializer[0].dims.__setitem__(0, 4))
        reject(lambda m: setattr(m.graph.initializer[0], "raw_data", b"bad"))
        reject(lambda m: m.graph.initializer[0].float_data.append(1.0))
        reject(lambda m: m.graph.initializer[0].int32_data.append(1))
        reject(lambda m: setattr(m.graph.initializer[0], "data_type", TensorProto.DOUBLE))
        reject(lambda m: setattr(m.graph.initializer[0], "data_location", TensorProto.EXTERNAL))
        reject(lambda m: m.graph.initializer[0].external_data.add(key="location", value="outside.bin"))
        reject(lambda m: m.graph.initializer[0].segment.CopyFrom(TensorProto.Segment(begin=0, end=1)))
        reject(lambda m: setattr(m.graph.initializer[0], "raw_data", b"\0\0\x80\x7f" * 35))
        reject(lambda m: setattr(m.graph.initializer[0], "raw_data", b"\0\0\xc0\x7f" * 35))
        reject(lambda m: m.graph.value_info.add().CopyFrom(m.graph.value_info[0]))
        reject(lambda m: m.metadata_props.add(key="duplicate", value="a") and m.metadata_props.add(key="duplicate", value="b"))
        data = base.SerializeToString()
        for payload in (b"", data[:-1], data[:len(data)//2], data + b"\x08\x08",
                        data + b"\x80", data + b"\x80" * 10 + b"\x02",
                        data + b"\x80\x06\x01", data + b"\x38\x01", data + b"\x3a\xff\xff\xff\xff\x7f"):
            inspect(payload, 2)
        utf = copy.deepcopy(base); utf.ClearField("producer_name")
        inspect(utf.SerializeToString() + b"\x12\x01\xff", 2)
        # Structurally safe unsupported graphs are not asserted to be valid.
        for op in ("Identity", "Relu", "NotAnOnnxOperator"):
            graph = helper.make_graph([helper.make_node(op, ["X"], ["Y"])], "outside_subset",
                                       [helper.make_tensor_value_info("X", TensorProto.FLOAT, [2, 3])],
                                       [helper.make_tensor_value_info("Y", TensorProto.FLOAT, [2, 3])])
            candidate = helper.make_model(graph, ir_version=8, opset_imports=[helper.make_opsetid("", 13)])
            if op != "NotAnOnnxOperator": onnx.checker.check_model(candidate, full_check=True)
            else:
                try: onnx.checker.check_model(candidate, full_check=True)
                except onnx.checker.ValidationError: pass
                else: raise AssertionError("invalid operator fixture unexpectedly passed ONNX checker")
            inspect(candidate, 1)
        # Unknown nested type fields cannot silently change a supported model.
        candidate = copy.deepcopy(base)
        tensor_type = candidate.graph.input[0].type
        tensor_type.ParseFromString(tensor_type.SerializeToString() + b"\x80\x06\x01")
        inspect(candidate, 2)
    print(f"PASS native AOT importer: {checks} snapshots, Python/ONNX parity, bounds, ownership and rejection")


if __name__ == "__main__":
    main()
