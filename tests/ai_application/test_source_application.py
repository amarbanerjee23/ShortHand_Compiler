"""Entire held-out dataset through actual ShortHand source and generated LLVM."""
import json
import os
import pathlib
import subprocess
import sys
import tempfile
root, compiler, tool = map(pathlib.Path, sys.argv[1:4])
sys.path.insert(0, str(root / 'scripts'))
from create_digit_application_fixture import create, read_split
flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if os.environ.get('SHORTHAND_MLIR_SANITIZERS') == 'ON' else []
def run(*args, stdin=None):
    r = subprocess.run(list(map(str, args)), input=stdin, text=True, capture_output=True, timeout=120,
                       env=dict(os.environ, ORT_DISABLE_TELEMETRY='1'))
    assert r.returncode == 0 and not any(s in r.stderr for s in ('AddressSanitizer', 'LeakSanitizer', 'runtime error:')), (args, r.returncode, r.stderr)
    return r.stdout
with tempfile.TemporaryDirectory(prefix='shorthand-real-digits-') as directory:
    work = pathlib.Path(directory)
    config = create(work)
    rows = read_split(root / 'tests/ai_application/data/optdigits.tes.csv')
    stdin = str(len(rows)) + '\n' + '\n'.join(' '.join(map(str, r[:-1])) for r in rows) + '\n'
    run(tool, 'application', config, work / 'native.json')
    expected = json.loads((work / 'native.json').read_text())['predictions']
    reference = [int(v) for v in run(compiler, work / 'digits.short', 'run', stdin=stdin).split()]
    assert reference == expected and sum(p == r[-1] for p, r in zip(reference, rows)) >= .85 * len(rows)
    ir = work / 'digits.ll'
    run(compiler, work / 'digits.short', 'compile-mlir', '--output', ir)
    for optimization in ('-O0', '-O2'):
        run(os.environ.get('SHORTHAND_LLVM_CLANG', 'clang++'), ir, optimization, *flags, '-o', work / 'digits')
        assert [int(v) for v in run(work / 'digits', stdin=stdin).split()] == expected
print('PASS 1797 real images: ShortHand interpreter, MLIR/LLVM O0/O2 and FP32 ONNX agree on all predictions')
