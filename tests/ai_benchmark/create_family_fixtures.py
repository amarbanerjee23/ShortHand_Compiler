#!/usr/bin/env python3
"""Create bounded ONNX family fixtures using only the Python standard library.

These are execution-contract fixtures, never standard-dataset or energy evidence.
"""
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/ai_energy"))
from onnx_fixture import integer, message, value_info  # noqa: E402


def tensor(name, shape, data_type, raw):
    body = b"".join(integer(1, n) for n in shape)
    body += integer(2, data_type) + message(8, name) + message(9, raw)
    return body


def node(inputs, output, op):
    return b"".join(message(1, name) for name in inputs) + message(2, output) + message(4, op)


def write_model(path, graph_name, nodes, initializers, input_shape, output_shape):
    graph = b"".join(message(1, item) for item in nodes) + message(2, graph_name)
    graph += b"".join(message(5, item) for item in initializers)
    graph += message(11, value_info("X", input_shape)) + message(12, value_info("Y", output_shape))
    model = integer(1, 9) + message(2, "ShortHand PR98 family fixture") + message(7, graph) + message(8, integer(2, 13))
    pathlib.Path(path).write_bytes(model)


def matmul(path, columns):
    rows = 4
    values = [((n % 11) - 5) / 16.0 for n in range(rows * columns)]
    weights = tensor("W", [rows, columns], 1, b"".join(struct.pack("<f", value) for value in values))
    write_model(path, f"family_matmul_{columns}", [node(["X", "W"], "Y", "MatMul")], [weights], [2, rows], [2, columns])


def quantized_roundtrip(path):
    scale = tensor("scale", [1], 1, struct.pack("<f", 0.125))
    zero = tensor("zero", [1], 2, b"\x80")  # UINT8 zero point 128.
    nodes = [node(["X", "scale", "zero"], "Xq", "QuantizeLinear"),
             node(["Xq", "scale", "zero"], "Y", "DequantizeLinear")]
    write_model(path, "quantized_roundtrip", nodes, [scale, zero], [2, 4], [2, 4])


def create(output):
    output = pathlib.Path(output)
    output.mkdir(parents=True, exist_ok=True)
    matmul(output / "retrieval.onnx", 3)
    matmul(output / "detection.onnx", 12)
    quantized_roundtrip(output / "quantized.onnx")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: create_family_fixtures.py OUTPUT_DIR")
    create(sys.argv[1])
    print("PASS PR98 deterministic retrieval detection and INT8-internal ONNX fixtures")
