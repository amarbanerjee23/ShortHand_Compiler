"""Native immutable capability contracts using independent standard-library ONNX fixtures."""
import argparse
import hashlib
import os
import pathlib
import struct
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/ai_energy'))
from onnx_fixture import integer, message, value_info


def model(op='Identity', extra=b'', ir=9):
    node = message(1, 'X') + message(2, 'Y') + message(4, op)
    graph = message(1, node) + message(2, 'immutable_contract')
    graph += message(11, value_info('X', [1, 4])) + message(12, value_info('Y', [1, 4])) + extra
    return integer(1, ir) + message(7, graph) + message(8, integer(2, 13))


def write_model(path, data):
    path.write_bytes(data)
    path.with_name(path.name + '.sha256').write_text(hashlib.sha256(data).hexdigest())


def fixtures(work):
    work.mkdir(parents=True, exist_ok=True)
    good = model()
    write_model(work / 'identity.onnx', good)
    write_model(work / 'negative.onnx', model('Neg'))
    node = message(1, 'X') + message(1, 'X') + message(2, 'Y') + message(4, 'Add')
    graph = message(1, node) + message(2, 'overflow')
    graph += message(11, value_info('X', [1, 4])) + message(12, value_info('Y', [1, 4]))
    write_model(work / 'overflow.onnx', integer(1, 9) + message(7, graph) + message(8, integer(2, 13)))
    for size, name in [(16 * 1024 * 1024, 'limit'), (64 * 1024, 'benchmark-64k'),
                       (1024 * 1024, 'benchmark-1m')]:
        # Padding is legitimate ModelProto doc_string, not trailing junk.
        padding = size - len(good) - len(message(6, b'x' * size)) + size
        value = good + message(6, b'x' * padding)
        assert len(value) == size
        write_model(work / (name + '.onnx'), value)
    external = integer(1, 4) + integer(2, 1) + message(8, 'W')
    external += message(13, message(1, 'location') + message(2, 'weights.bin')) + integer(14, 1)
    (work / 'weights.bin').write_bytes(struct.pack('<4f', 1, 1, 1, 1))
    bad = {
        'truncated': good[:-1], 'unknown-model-field': good + message(99, b''),
        'bad-ir': model(ir=11), 'duplicate-ir': good + integer(1, 9),
        'duplicate-graph': good + message(7, b''), 'training': good + message(20, b''),
        'function': good + message(25, b''), 'device': good + message(26, b''),
        'external-initializer': model(extra=message(5, external)),
        'external-sparse': model(extra=message(15, message(1, external))),
        'wrong-graph-wire': good + integer(7, 0), 'overflow-varint': b'\x08' + b'\xff' * 10,
        'bad-op': model('NotAnOnnxOperator'),
        'shape-mismatch': model().replace(value_info('X', [1, 4]), value_info('X', [1, 5])),
        'unknown-node': model(extra=message(1, message(99, b''))),
        'custom-domain': model(extra=message(1, message(7, 'custom'))),
        'external-location': model(extra=message(5, integer(14, 1))),
        'wrong-location-wire': model(extra=message(5, message(14, b''))),
        'tensor-segment': model(extra=message(5, message(3, b''))),
    }
    # Dependency-bearing tensors in every AttributeProto nesting form.
    for field in (5, 10, 22, 23, 6, 11):
        nested = external if field in (5, 10) else message(1, external) if field in (22, 23) else message(5, external)
        bad['external-attribute-' + str(field)] = model(extra=message(1, message(5, message(field, nested))))
    for name, data in bad.items():
        write_model(work / ('reject-' + name + '.onnx'), data)


def compile_probe(build, sdk, output, sanitizers=False):
    flags = []
    if sanitizers:
        symbols = subprocess.check_output(['nm', '-u', str(build / 'libshorthand_runtime.a')], text=True)
        if '__asan_' not in symbols or '__ubsan_' not in symbols:
            raise ValueError('instrumented runtime archive required')
        flags = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    command = [os.environ.get('CXX', 'clang++-18'), '-std=c++17', '-O2', *flags,
               '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
               str(ROOT / 'tests/ai_application/test_immutable_model.cpp'),
               str(build / 'libshorthand_runtime.a'), '-L' + str(sdk / 'lib'),
               '-Wl,-rpath,' + str(sdk / 'lib'), '-lonnxruntime', '-o', str(output)]
    subprocess.run(command, check=True, timeout=180)
    return command


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=pathlib.Path)
    parser.add_argument('--sdk', type=pathlib.Path)
    parser.add_argument('--executable', type=pathlib.Path)
    parser.add_argument('--fixtures', type=pathlib.Path)
    parser.add_argument('--sanitizers', action='store_true')
    args = parser.parse_args()
    if args.fixtures:
        fixtures(args.fixtures)
        return
    with tempfile.TemporaryDirectory(prefix='shorthand-immutable-') as tmp:
        work = pathlib.Path(tmp)
        fixtures(work)
        binary = args.executable.resolve() if args.executable else work / 'test'
        if not args.executable:
            compile_probe(args.build.resolve(), args.sdk.resolve(), binary, args.sanitizers)
        subprocess.run([str(binary), 'contracts', str(work)], check=True, timeout=240)


if __name__ == '__main__':
    main()
