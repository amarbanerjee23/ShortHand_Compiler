#!/usr/bin/env python3
"""Deterministic train-only centroid ONNX model and native digit application."""
import argparse
import csv
import hashlib
import json
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/ai_energy'))
from onnx_fixture import integer, message

PINS = {
    'optdigits.tra.csv': (563639, 'e1b683cc211604fe8fd8c4417e6a69f31380e0c61d4af22e93cc21e9257ffedd', 3823),
    'optdigits.tes.csv': (264712, '6ebb3d2fee246a4e99363262ddf8a00a3c41bee6014c373ed9d9216ba7f651b8', 1797),
}

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def write_json(path, value):
    path.write_text(json.dumps(value, sort_keys=True, indent=2) + '\n')

def read_split(path):
    size, digest, count = PINS[path.name]
    if path.is_symlink() or path.stat().st_size != size or sha(path) != digest:
        raise ValueError('pinned dataset integrity failure')
    rows = [list(map(int, row)) for row in csv.reader(path.open())]
    if len(rows) != count or any(len(r) != 65 or not 0 <= r[-1] <= 9 or any(not 0 <= v <= 16 for v in r[:-1]) for r in rows):
        raise ValueError('dataset shape, label or feature failure')
    return rows

def value_info(name, width):
    dims = message(1, message(2, 'batch')) + message(1, integer(1, width))
    return message(1, name) + message(2, message(1, integer(1, 1) + message(2, dims)))

def tensor(name, shape, values):
    return b''.join(integer(1, n) for n in shape) + integer(2, 1) + message(8, name) + message(9, struct.pack('<' + 'f' * len(values), *values))

def create(output, batch=16, threads=1, workers=2):
    output.mkdir(parents=True, exist_ok=True)
    train_path = ROOT / 'tests/ai_application/data/optdigits.tra.csv'
    test_path = ROOT / 'tests/ai_application/data/optdigits.tes.csv'
    train = read_split(train_path)
    read_split(test_path)  # Integrity only: no test labels enter model fitting.
    sums, counts = [[0] * 64 for _ in range(10)], [0] * 10
    for row in train:
        label = row[-1]
        counts[label] += 1
        for k, value in enumerate(row[:-1]):
            sums[label][k] += value
    centers = [[value / (16 * counts[label]) for value in sums[label]] for label in range(10)]
    weights = [2 * centers[label][k] for k in range(64) for label in range(10)]
    bias = [-sum(v * v for v in center) for center in centers]
    matmul = message(1, 'X') + message(1, 'W') + message(2, 'distance') + message(4, 'MatMul')
    add = message(1, 'distance') + message(1, 'B') + message(2, 'Y') + message(4, 'Add')
    graph = message(1, matmul) + message(1, add) + message(2, 'uci_train_only_centroids')
    graph += message(5, tensor('W', [64, 10], weights)) + message(5, tensor('B', [10], bias))
    graph += message(11, value_info('X', 64)) + message(12, value_info('Y', 10))
    model = output / 'digits.onnx'
    model.write_bytes(integer(1, 9) + message(2, 'ShortHand UCI train-only centroid fixture') + message(7, graph) + message(8, integer(2, 13)))
    q = dict(schema='shorthand.ai.cpu_qualification.config.v1', workload='uci_optdigits_centroid', model_path=str(model.resolve()),
             model_sha256=sha(model), input_shape=[batch, 64], output_shape=[batch, 10], threads=sorted(set([1, threads])),
             batch_size=batch, warmups=2, repetitions=2, trials=3, energy_source='unavailable', maximum_memory_bytes=1073741824)
    qpath = output / 'qualification.json'
    write_json(qpath, q)
    app = dict(schema='shorthand.ai.application.config.v1', qualification_config=str(qpath.resolve()), qualification_sha256=sha(qpath),
               dataset_path=str(test_path), dataset_sha256=sha(test_path), dataset_id='uci-optdigits-80-original-test',
               dataset_source='https://doi.org/10.24432/C50P49', dataset_license='CC-BY-4.0', dataset_split='original_author_separated_test',
               features=64, classes=10, top_k=3, threads=threads, workers=workers, request_timeout_ms=30000,
               input_min=0, input_max=16, offset=0, scale=0.0625, minimum_accuracy=0.85)
    write_json(output / 'application.json', app)
    # Standalone ShortHand source: frozen float64 language semantics, original
    # pixels via stdin, train-only weights, executable preprocessing/classifier/
    # postprocessing. This is a correctness oracle, not an FP32 speed comparison.
    source = ['int rows, row, i, label, best;', 'float pixel, score, maximum, values[64], weights[640], bias[10];']
    for index, value in enumerate(weights):
        source.append(f'weights[{index}] = {value:.17f};')
    for index, value in enumerate(bias):
        source.append(f'bias[{index}] = {value:.17f};')
    source += ['read rows;', 'row = 0;', 'loop row = 0, 1, rows {', 'i = 0;',
               'loop i = 0, 1, 64 { read pixel; values[i] = pixel / 16.0; }',
               'label = 0; best = 0; maximum = -1000000.0;', 'loop label = 0, 1, 10 {',
               'score = bias[label]; i = 0;',
               'loop i = 0, 1, 64 { score = score + values[i] * weights[i * 10 + label]; }',
               'if score > maximum { maximum = score; best = label; }',
               '}', 'print best;', '}']
    (output / 'digits.short').write_text('\n'.join(source) + '\n')
    write_json(output / 'provenance.json', dict(training_sha256=sha(train_path), test_sha256=sha(test_path), model_sha256=sha(model),
               generator_sha256=sha(pathlib.Path(__file__)), train_rows=3823, test_rows=1797, test_used_for_training=False,
               algorithm='nearest_class_centroid_squared_euclidean', precision='float32', minimum_accuracy=0.85,
               scope='small real digit-classification workload; no broad AI or carbon claim'))
    return output / 'application.json'

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=pathlib.Path)
    parser.add_argument('--batch', type=int, default=16)
    parser.add_argument('--threads', type=int, default=1)
    args = parser.parse_args()
    if not 1 <= args.batch <= 1024 or not 1 <= args.threads <= 256:
        parser.error('invalid batch or threads')
    print(create(args.output, args.batch, args.threads))
