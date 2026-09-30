"""Real SDK cache regression; invokes the built, serialized C ABI archive."""
import os
import pathlib
import struct
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/ai_energy'))
from onnx_fixture import integer, message, value_info, external_model


def main():
    build, sdk = map(lambda s: pathlib.Path(s).resolve(), sys.argv[1:3])
    clang = os.environ.get('CXX', 'clang++-18')
    with tempfile.TemporaryDirectory(prefix='shorthand-cache-') as tmp:
        work = pathlib.Path(tmp)
        # Identity and Neg intentionally differ but both have fixed FP32 [1,1] IO.
        for name, op in [('identity', 'Identity'), ('negative', 'Neg')]:
            node = message(1, 'X') + message(2, 'Y') + message(4, op)
            graph = message(1, node) + message(2, 'cache_test')
            graph += message(11, value_info('X', [1, 1])) + message(12, value_info('Y', [1, 1]))
            (work / (name + '.onnx')).write_bytes(integer(1, 9) + message(7, graph) + message(8, integer(2, 13)))
        external_model(work / 'external.onnx', rows=1, columns=1)
        (work / 'weights.bin').write_bytes(struct.pack('<f', 2))
        command = [clang, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-pthread',
                   '-I' + str(ROOT / 'Compiler_new_ws/Short_Hand/src'),
                   str(ROOT / 'tests/ai_application/test_compiled_cache.cpp'),
                   str(build / 'libshorthand_runtime.a'), '-L' + str(sdk / 'lib'),
                   '-Wl,-rpath,' + str(sdk / 'lib'), '-lonnxruntime', '-o', str(work / 'test')]
        subprocess.run(command, check=True, timeout=180)
        subprocess.run([str(work / 'test'), *(str(work / name) for name in
                       ('identity.onnx', 'negative.onnx', 'external.onnx', 'mutable.onnx'))], check=True, timeout=180)


if __name__ == '__main__':
    main()
