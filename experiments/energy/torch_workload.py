#!/usr/bin/env python3
"""PyTorch state-of-practice source baseline for the frozen Optdigits workload.

The process contract matches source_workload.py: model JSON via --model, the
frozen repetitions+pixels stream on stdin, and final predictions plus a checksum
on stdout. Imports, framework startup and torch.compile setup are deliberately
inside the whole-process measurement boundary used by state_of_practice.py.
"""
import argparse
import json
import pathlib
import sys


def worker(model_path, mode):
    try:
        import torch
    except Exception as exc:
        raise RuntimeError('PyTorch baseline requested but torch is unavailable') from exc

    if not hasattr(torch, 'set_num_threads'):
        raise RuntimeError('invalid PyTorch installation')
    torch.set_num_threads(1)
    try:
        torch.set_num_interop_threads(1)
    except RuntimeError:
        pass

    model = json.loads(pathlib.Path(model_path).read_text())
    tokens = sys.stdin.buffer.read().decode('ascii').split()
    if len(tokens) != 115009:
        raise ValueError('invalid frozen workload input')
    repetitions = int(tokens[0])
    if not 1 <= repetitions <= 10000 or float(tokens[0]) != repetitions:
        raise ValueError('invalid repetition count')

    raw = torch.tensor([float(v) for v in tokens[1:]], dtype=torch.float64).reshape(1797, 64)
    weights = torch.tensor(model['weights'], dtype=torch.float64).reshape(64, 10)
    bias = torch.tensor(model['bias'], dtype=torch.float64)

    def classify(values):
        return torch.argmax((values / 16.0) @ weights + bias, dim=1)

    if mode == 'compile':
        if not hasattr(torch, 'compile'):
            raise RuntimeError('torch.compile is unavailable in this PyTorch build')
        classify = torch.compile(classify, fullgraph=True)

    checksum = 0
    predictions = None
    with torch.inference_mode():
        for iteration in range(repetitions):
            shift = iteration % raw.shape[0]
            segments = (raw[shift:], raw[:shift]) if shift else (raw,)
            parts = [classify(values) for values in segments]
            predictions = torch.cat(parts)
            checksum += int(predictions.sum().item())

    values = predictions.tolist()
    sys.stdout.write('\n'.join(map(str, values + [checksum])) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', type=pathlib.Path, required=True)
    parser.add_argument('--mode', choices=['eager', 'compile'], required=True)
    parser.add_argument('--version', action='store_true', help='print PyTorch version and exit')
    args = parser.parse_args()
    if args.version:
        import torch
        print(torch.__version__)
        return
    worker(args.model, args.mode)


if __name__ == '__main__':
    main()
