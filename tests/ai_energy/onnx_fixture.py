"""Small standard-library ONNX wire fixture builder; no ONNX/Python runtime dependency."""
import hashlib
import pathlib
import struct


def varint(n):
    out = bytearray()
    while n > 127:
        out.append((n & 127) | 128)
        n >>= 7
    out.append(n)
    return bytes(out)


def integer(field, value):
    return varint(field << 3) + varint(value)


def message(field, value):
    if isinstance(value, str):
        value = value.encode()
    return varint((field << 3) | 2) + varint(len(value)) + value


def value_info(name, shape):
    dimensions = b"".join(message(1, integer(1, n)) for n in shape)
    tensor_type = integer(1, 1) + message(2, dimensions)
    return message(1, name) + message(2, message(1, tensor_type))


def external_model(path, rows=4, columns=3, location="weights.bin"):
    path = pathlib.Path(path)
    external = b"".join(message(13, message(1, k) + message(2, v)) for k, v in
                        [("location", location), ("offset", "0"), ("length", str(rows * columns * 4))])
    weights = integer(1, rows) + integer(1, columns) + integer(2, 1) + message(8, "W") + external + integer(14, 1)
    node = message(1, "X") + message(1, "W") + message(2, "Y") + message(4, "MatMul")
    graph = message(1, node) + message(2, "synthetic_external_weights") + message(5, weights)
    graph += message(11, value_info("X", [1, rows])) + message(12, value_info("Y", [1, columns]))
    path.write_bytes(integer(1, 9) + message(2, "ShortHand deterministic scalability fixture") + message(7, graph) + message(8, integer(2, 13)))


def write_weights(path, count):
    pattern = b"".join(struct.pack("<f", (n % 17 - 8) / 1000) for n in range(4096))
    remaining, digest = count * 4, hashlib.sha256()
    with pathlib.Path(path).open("wb") as stream:
        while remaining:
            chunk = pattern[:min(remaining, len(pattern))]
            stream.write(chunk)
            digest.update(chunk)
            remaining -= len(chunk)
    return digest.hexdigest()
