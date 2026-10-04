"""Verify diagnostic isolation and actual SDK execution, including failures."""
import os
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from create_digit_application_fixture import create


def main():
    build, sdk = (pathlib.Path(p).resolve() for p in sys.argv[1:])
    for name, instrumented in [('shorthand_runtime', False), ('shorthand_runtime_profiled', True)]:
        symbols = subprocess.check_output(['nm', '-C', str(build / ('lib' + name + '.a'))], text=True)
        if ('shorthand::runtime_profile::' in symbols) != instrumented:
            raise ValueError('profiling symbols must be isolated to the diagnostic archive')
    with tempfile.TemporaryDirectory(prefix='shorthand-phases-') as tmp:
        work = pathlib.Path(tmp)
        create(work, batch=16, threads=1)
        binary = work / 'test'
        subprocess.run([os.environ.get('CXX', 'clang++-18'), '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-pthread', '-DSHORTHAND_RUNTIME_PHASE_PROFILE=1',
                        '-I' + str(ROOT / 'Compiler_new_ws/Short_Hand/src'),
                        str(ROOT / 'tests/ai_application/test_runtime_phase_profile.cpp'),
                        str(build / 'libshorthand_runtime_profiled.a'),
                        '-L' + str(sdk / 'lib'), '-Wl,-rpath,' + str(sdk / 'lib'), '-lonnxruntime', '-o', str(binary)],
                       check=True, timeout=180)
        subprocess.run([str(binary), str(work / 'digits.onnx')], check=True, timeout=180)
    print('PASS phase profile isolation: production archive has no diagnostic symbols')


if __name__ == '__main__':
    main()
