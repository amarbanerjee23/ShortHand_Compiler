#!/usr/bin/env python3
import argparse
import copy
import json
import pathlib
import shutil
import tempfile
import unittest
import immutable_model_evidence as evidence


class EvidenceTests(unittest.TestCase):
    bundle = None

    def test_trace_window_and_worker_detection(self):
        start = '100 write(1<pipe:[1]>, "MODEL_WARM_BEGIN\\n", 17) = 17\n'
        end = '100 write(1<pipe:[1]>, "MODEL_WARM_END\\n", 15) = 15\n'
        read = '101 read(3</models/m.onnx>, "data", 4) = 4\n'
        opened = '101 openat(AT_FDCWD, "/models/m.onnx", O_RDONLY) = 3</models/m.onnx>\n'
        exited = '100 +++ exited with 0 +++\n'
        self.assertEqual(evidence.trace_counts(opened + read + start + end + exited)['model_io_syscalls'], 0)
        counts = evidence.trace_counts(start + opened + read + end + exited)
        self.assertEqual(counts, dict(model_io_syscalls=2, model_opens=1, model_reads=1))
        for invalid in (start + exited, start + start + end + exited, start + end, end + start + exited):
            with self.assertRaises(ValueError):
                evidence.trace_counts(invalid)

    def test_complete_bundle(self):
        report = evidence.replay(self.bundle, (self.bundle / 'manifest.sha256').read_text().strip())
        self.assertTrue(report['success'])
        self.assertEqual(len(report['rows']), 3)

    def test_corruption_cannot_write_accepted_report(self):
        mutations = ('omit_pair', 'swap_order', 'unhash_stdout', 'failed_call', 'wrong_command', 'invent_energy', 'missing_trace', 'missing_sources', 'raw_tamper')
        for mutation in mutations:
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as temporary:
                out = pathlib.Path(temporary)
                # Large immutable binary/SDK/fixture files are read-only hardlinks;
                # changed records are replaced atomically before modifying them.
                shutil.copytree(self.bundle, out, dirs_exist_ok=True, copy_function=__import__('os').link)
                (out / 'report.json').unlink(missing_ok=True)
                (out / 'IMMUTABLE.md').unlink(missing_ok=True)
                manifest = copy.deepcopy(json.loads((out / 'manifest.json').read_text()))
                def replace(name, value):
                    (out / name).unlink()
                    evidence.write(out / name, value)
                    manifest['hashes'][name] = evidence.digest(out / name)
                if mutation == 'omit_pair':
                    manifest['observations'].pop()
                elif mutation == 'swap_order':
                    manifest['observations'][0], manifest['observations'][1] = manifest['observations'][1], manifest['observations'][0]
                elif mutation == 'unhash_stdout':
                    manifest['hashes'].pop('identity-0-immutable.stdout')
                elif mutation == 'failed_call':
                    replace('identity-0-immutable.exit.json', dict(returncode=1, elapsed_seconds=1))
                elif mutation == 'wrong_command':
                    name = 'identity-0-immutable.command.json'
                    value = json.loads((out / name).read_text()); value[-1] = '1'
                    replace(name, value)
                elif mutation == 'invent_energy':
                    name = 'identity-energy-immutable.energy.json'
                    value = json.loads((out / name).read_text())
                    value.update(available=False, hardware_measured_joules=1)
                    replace(name, value)
                elif mutation == 'missing_trace':
                    manifest['traces'].pop()
                elif mutation == 'missing_sources':
                    for name in manifest['sources']:
                        manifest['hashes'].pop(name)
                    manifest['sources'] = []
                else:
                    name = 'trace-immutable.strace'
                    (out / name).unlink(); (out / name).write_text('tampered')
                (out / 'manifest.json').unlink()
                evidence.write(out / 'manifest.json', manifest)
                with self.assertRaises((ValueError, KeyError, OSError)):
                    evidence.replay(out, evidence.digest(out / 'manifest.json'))
                self.assertFalse((out / 'report.json').exists())
                self.assertFalse((out / 'IMMUTABLE.md').exists())


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--bundle', required=True, type=pathlib.Path)
    args, remaining = parser.parse_known_args()
    EvidenceTests.bundle = args.bundle.resolve()
    unittest.main(argv=[__file__, *remaining])
