#!/usr/bin/env python3
"""Correctness/negative tests; all fabricated energy is unit-test data only."""
import argparse
import copy
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest

import campaign
import state_of_practice
import runtime_state_of_practice
from source_workload import prepare


class Experiments(unittest.TestCase):
    def test_signed_results_and_compilation_amortization(self):
        compare = campaign.energy_statistics.compare
        a = compare([1., 2., 3., 4.], [2., 4., 6., 8.])
        self.assertEqual(a['savings_percent'], 50)
        self.assertEqual(a['paired_bootstrap_95_percent_interval'], [50, 50])
        self.assertEqual(compare([2.] * 4, [1.] * 4)['savings_percent'], -100)
        self.assertEqual(compare([1.] * 4, [1.] * 4)['interpretation'], 'inconclusive')
        self.assertEqual(compare([.99] * 4, [1.] * 4, uncertainty_percent=2)['interpretation'], 'inconclusive')
        self.assertEqual(campaign.energy_statistics.break_even(100, 5, 10, 1797)['break_even_runs'], 20)
        self.assertIsNone(campaign.energy_statistics.break_even(100, 10, 5, 1797)['break_even_runs'])
        for a, b in [([1.] * 3, [2.] * 3), ([1.] * 4, [2.] * 5), ([0.] * 4, [1.] * 4), ([float('nan')] * 4, [1.] * 4)]:
            with self.assertRaises(ValueError):
                compare(a, b)

    def test_pinned_source_and_optimized_python_agree(self):
        with tempfile.TemporaryDirectory() as temp:
            out = pathlib.Path(temp)
            prepare(out / 'fixture', 2)
            expected = campaign.load(out / 'fixture/expected.json')
            self.assertEqual(expected['completed'], 3594)
            self.assertGreaterEqual(expected['accuracy'], .85)
            for baseline in ('scalar', 'numpy'):
                trial = campaign.command([sys.executable, pathlib.Path(__file__).with_name('source_workload.py'),
                    '--model', out / 'fixture/model.json', '--baseline', baseline], out, baseline,
                    expected['completed'], out / 'fixture/input.txt')
                campaign.check_output(out / trial['stdout'], expected)
            broken = copy.deepcopy(expected)
            broken['checksum'] += 1
            with self.assertRaises(ValueError):
                campaign.check_output(out / 'numpy.stdout', broken)

    def test_prepare_freezes_every_cell_and_rejects_tampering(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp) / 'plan'
            args = argparse.Namespace(output=root, source_repetitions=1, runtime_repetitions=1,
                source_pairs=4, runtime_pairs=4, compile_repetitions=1, source_baseline='numpy',
                mode='execution_only', instrument=None, meter_csv=None)
            campaign.prepare(args)
            plan = campaign.load(root / 'plan.json')
            campaign.verify_files(root, plan['hashes'])
            missing = copy.deepcopy(plan)
            missing['runtime_cases'].pop()
            with self.assertRaises(ValueError):
                campaign.validate_plan(missing)
            fake = copy.deepcopy(plan)
            fake.update(mode='calibrated_energy', meter_csv='missing.csv')
            with self.assertRaises(ValueError):
                campaign.validate_plan(fake)
            (root / 'source/model.json').write_text('{}')
            with self.assertRaises(ValueError):
                campaign.verify_files(root, plan['hashes'])
            with self.assertRaises(ValueError):
                campaign.verify_files(root, {'../escape': '0' * 64})

    def test_measured_mode_never_substitutes_estimates(self):
        with tempfile.TemporaryDirectory() as temp:
            args = argparse.Namespace(output=pathlib.Path(temp) / 'plan', source_repetitions=1,
                runtime_repetitions=1, source_pairs=30, runtime_pairs=10, compile_repetitions=1,
                mode='calibrated_energy', instrument=None, meter_csv=None)
            with self.assertRaises(ValueError):
                campaign.prepare(args)

    def test_external_state_of_practice_version_is_verified(self):
        with tempfile.TemporaryDirectory() as temp:
            runner = pathlib.Path(temp) / 'peer-runner'
            runner.write_text('#!/bin/sh\nif [ "$1" = "--version" ]; then echo 1.2.3; exit 0; fi\nexit 2\n')
            runner.chmod(0o755)
            spec = state_of_practice._external(runner, '1.2.3', 'rust')
            self.assertEqual(spec['version'], '1.2.3')
            self.assertEqual(spec['sha256'], campaign.sha(runner))
            with self.assertRaises(ValueError):
                state_of_practice._external(runner, '9.9.9', 'rust')

    def test_state_of_practice_matrix_is_fail_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp) / 'sota-plan'
            state_of_practice.prepare(argparse.Namespace(output=root, mode='execution_only', profile='core',
                repetitions=1, pairs=4, instrument=None, meter_csv=None, pytorch_python=None,
                rust_command=None, rust_version=None, mojo_command=None, mojo_version=None))
            plan = campaign.load(root / 'plan.json')
            self.assertEqual(tuple(plan['baselines']), state_of_practice.CORE_BASELINES)
            state_of_practice.validate_plan(plan)
            incomplete = copy.deepcopy(plan)
            incomplete['profile'] = 'full'
            with self.assertRaises(ValueError):
                state_of_practice.validate_plan(incomplete)
            measured = copy.deepcopy(plan)
            measured.update(mode='calibrated_energy', pairs=30, meter_csv=None)
            with self.assertRaises(ValueError):
                state_of_practice.validate_plan(measured)


def compiled_integration(compiler, clang, tool):
    """Mandatory in the existing unsanitized MLIR lane; never an energy result."""
    with tempfile.TemporaryDirectory() as temp:
        root = pathlib.Path(temp)
        campaign.prepare(argparse.Namespace(output=root / 'plan', source_repetitions=2,
            runtime_repetitions=1, source_pairs=4, runtime_pairs=4, compile_repetitions=1,
            source_baseline='numpy', mode='execution_only', instrument=None, meter_csv=None))
        plan_path = root / 'plan/plan.json'
        campaign.run(argparse.Namespace(plan=plan_path, plan_sha256=campaign.sha(plan_path),
            compiler=compiler, clang=clang, tool=tool, output=root / 'results'))
        result = campaign.load(root / 'results/summary.json')
        if result['energy_evidence_qualified'] or result['source_energy'] is not None or len(result['runtime_cells']) != 5:
            raise AssertionError('invalid execution-only campaign result')
        report = campaign.load(root / 'results/source/source.json')
        if len(report['pairs']) != 4 or any(p[k]['completed'] != 3594 for p in report['pairs'] for k in ('native', 'python')):
            raise AssertionError('incomplete compiled-source smoke experiment')
        digest = campaign.sha(root / 'results/manifest.json')
        (root / 'results/source/pair-0-native.stdout').write_text('0\n')
        try:
            campaign.analyze(root / 'results', digest, tool)
        except ValueError:
            pass
        else:
            raise AssertionError('tampered campaign accepted')

        state_of_practice.prepare(argparse.Namespace(output=root / 'sota-plan', mode='execution_only', profile='core',
            repetitions=1, pairs=4, instrument=None, meter_csv=None, pytorch_python=None,
            rust_command=None, rust_version=None, mojo_command=None, mojo_version=None))
        sota_plan = root / 'sota-plan/plan.json'
        state_of_practice.run(argparse.Namespace(plan=sota_plan, plan_sha256=campaign.sha(sota_plan),
            compiler=compiler, clang=clang, tool=tool, output=root / 'sota-results'))
        sota = campaign.load(root / 'sota-results/sota-summary.json')
        if sota['energy_evidence_qualified'] or sota['full_matrix_qualified'] or tuple(sota['baselines']) != state_of_practice.CORE_BASELINES:
            raise AssertionError('invalid core state-of-practice smoke result')
        if {cell['baseline'] for cell in sota['cells']} != set(state_of_practice.CORE_BASELINES):
            raise AssertionError('missing optimized C++ or NumPy state-of-practice control')
        onnx_root = os.environ.get('ONNXRUNTIME_ROOT')
        if not onnx_root:
            raise AssertionError('ONNXRUNTIME_ROOT is required for the independent C++/ONNX experiment smoke')
        runtime_state_of_practice.smoke(clang, tool, onnx_root)
    print('PASS execution-only campaigns: compiled FP64, five FP32 ORT cells, C++17/NumPy and independent C++/ONNX controls, replay and tamper rejection')


def native_meter_integration(tool):
    """Exercise existing C++ collector using explicit synthetic test-only trace."""
    with tempfile.TemporaryDirectory(prefix='synthetic-energy-test-') as temp:
        out = pathlib.Path(temp)
        trace, instrument = out / 'meter.csv', out / 'instrument.json'
        start = 1780000000.0
        trace.write_text('unix_time_s,power_w\n' + ''.join(f'{start + n * .05:.2f},10\n' for n in range(61)))
        campaign.write(instrument, dict(id='synthetic-test-only', calibration_id='unit-test-only',
            calibration_date='2026-01-01', validation_ref='synthetic-test-not-evidence', boundary='whole_host_ac',
            isolation='unit-test-only', uncertainty_percent=1))
        policy = campaign.load(campaign.ROOT / 'tests/ai_application/comparison_measurement_policy.json')
        trial = dict(start_unix_seconds=start + .5, end_unix_seconds=start + 2.5, elapsed_ms=2000, completed=10)
        campaign.attach_energy(tool, out, [trial], trace, instrument, policy)
        if abs(trial['energy']['joules'] - 20) > 1e-6:
            raise AssertionError('native meter integration differs')
        sparse = copy.deepcopy(trial)
        sparse.update(start_unix_seconds=start, end_unix_seconds=start + .01, elapsed_ms=10)
        try:
            campaign.attach_energy(tool, out, [sparse], trace, instrument, policy)
        except ValueError:
            pass
        else:
            raise AssertionError('short measurement window accepted')
    print('PASS existing native meter integration and sparse-window rejection (synthetic unit test only)')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler')
    parser.add_argument('--clang')
    parser.add_argument('--tool')
    args = parser.parse_args()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(Experiments))
    if not result.wasSuccessful():
        sys.exit(1)
    if args.compiler:
        if not args.clang or not args.tool:
            parser.error('--compiler requires --clang and --tool')
        compiled_integration(args.compiler, args.clang, args.tool)
    if args.tool:
        native_meter_integration(args.tool)
