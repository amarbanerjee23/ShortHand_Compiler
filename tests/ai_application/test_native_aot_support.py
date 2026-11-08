"""Standalone AOT support contracts, without an ONNX Runtime SDK."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    parser.add_argument('--sanitizers', action='store_true')
    args = parser.parse_args()
    flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if args.sanitizers else []
    src = ROOT / 'Compiler_new_ws/Short_Hand/src'
    with tempfile.TemporaryDirectory(prefix='shorthand-aot-support-') as tmp:
        binary = Path(tmp) / 'support'
        subprocess.run([args.cxx, '-std=c++17', '-O2', '-fno-fast-math', '-ffp-contract=off',
                        '-Wall', '-Wextra', '-Wpedantic', '-Werror', *flags, '-pthread',
                        '-I'+str(src), str(ROOT/'tests/ai_application/native_aot_support_probe.cpp'),
                        str(src/'aot/AotSupport.cpp'), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], text=True, capture_output=True, timeout=60)
        assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
        assert not any(x in result.stderr for x in ('Sanitizer', 'runtime error:')), result.stderr
        print(result.stdout, end='')

if __name__ == '__main__':
    main()
