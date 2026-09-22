#!/usr/bin/env python3
"""Real held-out FP64 classifier baselines for the Shorthand energy study."""
import argparse
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from create_digit_application_fixture import read_split, write_json


def prepare(output, repetitions):
    """Fit only the pinned training split; freeze identical FP64 weights/input."""
    train = read_split(ROOT / 'tests/ai_application/data/optdigits.tra.csv')
    test = read_split(ROOT / 'tests/ai_application/data/optdigits.tes.csv')
    sums, counts = [[0] * 64 for _ in range(10)], [0] * 10
    for row in train:
        counts[row[-1]] += 1
        for k, value in enumerate(row[:-1]):
            sums[row[-1]][k] += value
    centers = [[v / (16 * counts[c]) for v in sums[c]] for c in range(10)]
    weights = [2 * centers[c][k] for k in range(64) for c in range(10)]
    bias = [-sum(v * v for v in center) for center in centers]
    predictions = []
    for row in test:
        scores = []
        for label in range(10):
            score = bias[label]
            for k in range(64):
                score += (row[k] / 16.0) * weights[k * 10 + label]
            scores.append(score)
        predictions.append(max(range(10), key=lambda c: scores[c]))
    accuracy = sum(p == r[-1] for p, r in zip(predictions, test)) / len(test)
    if accuracy < .85:
        raise ValueError('held-out accuracy below predeclared 85%')
    output.mkdir(parents=True, exist_ok=False)
    write_json(output / 'model.json', dict(weights=weights, bias=bias, precision='float64'))
    final_shift = (repetitions - 1) % len(test)
    ordered = predictions[final_shift:] + predictions[:final_shift]
    write_json(output / 'expected.json', dict(predictions=ordered, accuracy=accuracy,
               completed=len(test) * repetitions, checksum=sum(predictions) * repetitions))
    (output / 'input.txt').write_text(str(repetitions) + '\n' + '\n'.join(
        ' '.join(map(str, row[:-1])) for row in test) + '\n')
    source = ['int repeats, iteration, row, inputrow, shift, i, label, best, checksum, predictions[1797];',
              'float raw0[65536], raw1[49472], values[64], weights[640], bias[10], score, maximum;']
    source += [f'weights[{i}] = {v:.17f};' for i, v in enumerate(weights)]
    source += [f'bias[{i}] = {v:.17f};' for i, v in enumerate(bias)]
    source += ['read repeats; i = 0;', 'loop i = 0, 1, 65536 { read score; raw0[i] = score; }',
               'i = 0;', 'loop i = 0, 1, 49472 { read score; raw1[i] = score; }',
               'checksum = 0; iteration = 0; shift = 0;', 'loop iteration = 0, 1, repeats {',
               'row = 0;', 'loop row = 0, 1, 1797 {', 'i = 0;',
               'inputrow = row + shift; if inputrow >= 1797 { inputrow = inputrow - 1797; }',
               'if inputrow < 1024 {',
               'loop i = 0, 1, 64 { values[i] = raw0[inputrow * 64 + i] / 16.0; }',
               '} else {',
               'loop i = 0, 1, 64 { values[i] = raw1[(inputrow - 1024) * 64 + i] / 16.0; }', '}',
               'label = 0; best = 0; maximum = -1000000.0;',
               'loop label = 0, 1, 10 {', 'score = bias[label]; i = 0;',
               'loop i = 0, 1, 64 { score = score + values[i] * weights[i * 10 + label]; }',
               'if score > maximum { maximum = score; best = label; }', '}',
               'predictions[row] = best; checksum = checksum + best;', '}',
               'shift = shift + 1; if shift >= 1797 { shift = 0; }', '}',
               'row = 0;', 'loop row = 0, 1, 1797 { print predictions[row]; }', 'print checksum;']
    (output / 'classifier.short').write_text('\n'.join(source) + '\n')


def _numpy_inputs(model_path):
    import numpy as np
    if sys.version_info[:2] != (3, 12) or np.__version__ != '2.3.5':
        raise ValueError('baseline requires CPython 3.12 and pinned NumPy 2.3.5')
    model = json.loads(model_path.read_text())
    data = np.fromstring(sys.stdin.buffer.read().decode('ascii'), sep=' ', dtype=np.float64)
    repetitions = int(data[0])
    if len(data) != 115009 or repetitions != data[0] or not 1 <= repetitions <= 10000:
        raise ValueError('invalid workload input')
    raw = data[1:].reshape(1797, 64)
    weights = np.asarray(model['weights'], dtype=np.float64).reshape(64, 10)
    bias = np.asarray(model['bias'], dtype=np.float64)
    return np, repetitions, raw, weights, bias


def worker(model_path, baseline):
    np, repetitions, raw, weights, bias = _numpy_inputs(model_path)
    checksum = 0
    if baseline == 'numpy':
        for iteration in range(repetitions):
            shift = iteration % len(raw)
            segments = (raw[shift:], raw[:shift]) if shift else (raw,)
            parts = [np.argmax((values / 16.0) @ weights + bias, axis=1) for values in segments]
            predictions = np.concatenate(parts)
            checksum += int(predictions.sum())
        predictions = predictions.tolist()
    elif baseline in ('torch-eager', 'torch-compile'):
        import torch
        torch.set_num_threads(1)
        try:
            torch.set_num_interop_threads(1)
        except RuntimeError:
            pass
        values = torch.from_numpy(np.ascontiguousarray(raw)).to(dtype=torch.float64)
        torch_weights = torch.from_numpy(np.ascontiguousarray(weights)).to(dtype=torch.float64)
        torch_bias = torch.from_numpy(np.ascontiguousarray(bias)).to(dtype=torch.float64)

        def classify(batch):
            return torch.argmax((batch / 16.0) @ torch_weights + torch_bias, dim=1)

        classify_impl = torch.compile(classify, fullgraph=True, dynamic=False) if baseline == 'torch-compile' else classify
        for iteration in range(repetitions):
            shift = iteration % len(raw)
            segments = (values[shift:], values[:shift]) if shift else (values,)
            parts = [classify_impl(segment) for segment in segments]
            predictions = torch.cat(parts)
            checksum += int(predictions.sum().item())
        predictions = predictions.tolist()
    else:
        raw, weights, bias = raw.tolist(), weights.tolist(), bias.tolist()
        for iteration in range(repetitions):
            predictions = []
            for index in range(len(raw)):
                row = raw[(index + iteration) % len(raw)]
                values = [v / 16.0 for v in row]
                scores = []
                for label in range(10):
                    score = bias[label]
                    for i in range(64):
                        score += values[i] * weights[i][label]
                    scores.append(score)
                predictions.append(max(range(10), key=lambda c: scores[c]))
            checksum += sum(predictions)
    sys.stdout.write('\n'.join(map(str, predictions + [checksum])) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', type=pathlib.Path, required=True)
    parser.add_argument('--baseline', choices=['numpy', 'scalar', 'torch-eager', 'torch-compile'], default='numpy')
    args = parser.parse_args()
    worker(args.model, args.baseline)
