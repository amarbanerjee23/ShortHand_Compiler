"""Real SDK cache regression; invokes the built, serialized C ABI archive."""
import os
import argparse
import pathlib
import struct
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/ai_energy'))
from onnx_fixture import integer, message, value_info, external_model


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=pathlib.Path)
    parser.add_argument('sdk', type=pathlib.Path)
    parser.add_argument('--sanitizers', action='store_true')
    args = parser.parse_args()
    build, sdk = args.build.resolve(), args.sdk.resolve()
    clang = os.environ.get('CXX', 'clang++-18')
    flags = []
    if args.sanitizers:
        symbols = subprocess.check_output(['nm', '-u', str(build / 'libshorthand_runtime.a')], text=True)
        if '__asan_' not in symbols or '__ubsan_' not in symbols:
            raise ValueError('sanitizer test requires an instrumented runtime archive')
        flags = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    with tempfile.TemporaryDirectory(prefix='shorthand-cache-') as tmp:
        work = pathlib.Path(tmp)
        # Identity and Neg intentionally differ but both have fixed FP32 [1,1] IO.
        for name, op, size in [('identity', 'Identity', 1), ('negative', 'Neg', 1),
                               ('vector-65536', 'Identity', 65536), ('vector-65537', 'Identity', 65537)]:
            node = message(1, 'X') + message(2, 'Y') + message(4, op)
            graph = message(1, node) + message(2, 'cache_test')
            graph += message(11, value_info('X', [1, size])) + message(12, value_info('Y', [1, size]))
            model = integer(1, 9) + message(7, graph) + message(8, integer(2, 13))
            # ModelProto doc_string padding gives Identity/Neg identical file
            # sizes; the test also preserves mtime across replacement.
            model += message(6, 'x' * (512 - len(model) - 3))
            assert len(model) == 512
            (work / (name + '.onnx')).write_bytes(model)
        model = (work / 'identity.onnx').read_bytes()
        for name, size in [('limit', 16 * 1024 * 1024), ('over-limit', 16 * 1024 * 1024 + 1)]:
            padded = model + message(6, 'p' * (size - len(model) - 5))
            assert len(padded) == size
            (work / (name + '.onnx')).write_bytes(padded)
        external_model(work / 'external.onnx', rows=1, columns=1)
        (work / 'weights.bin').write_bytes(struct.pack('<f', 2))
        command = [clang, '-std=c++17', '-O1' if args.sanitizers else '-O2', *flags, '-Wall', '-Wextra', '-Werror', '-pthread',
                   '-I' + str(ROOT / 'Compiler_new_ws/Short_Hand/src'),
                   str(ROOT / 'tests/ai_application/test_compiled_cache.cpp'),
                   str(build / 'libshorthand_runtime.a'), '-L' + str(sdk / 'lib'),
                   '-Wl,-rpath,' + str(sdk / 'lib'), '-lonnxruntime', '-o', str(work / 'test')]
        subprocess.run(command, check=True, timeout=180)
        subprocess.run([str(work / 'test'), *(str(work / name) for name in
                       ('identity.onnx', 'negative.onnx', 'external.onnx', 'mutable.onnx'))], check=True, timeout=240)


if __name__ == '__main__':
    main()
