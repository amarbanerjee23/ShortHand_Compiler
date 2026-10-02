#!/usr/bin/env python3
import copy
import json
import pathlib
import tempfile
import unittest
from unittest.mock import Mock, patch
import generated_infer_evidence as evidence


def sample(batch=1):
    return dict(schema='shorthand.generated_infer.sample.v1', success=True, batch=batch,
                threads=1, iterations=8, warmups=8, completed_calls=16,
                completed_vectors=16*batch, block_elapsed_ms=[1.0, 1.1], cold_session_ms=2.0)


class GeneratedEvidenceTest(unittest.TestCase):
    def bundle(self, out):
        manifest = dict(schema='shorthand.generated_infer.bundle.v1', runners=['head', 'direct'],
                        rounds=1, iterations=8, blocks=2, workload='test fixture', observations=[], hashes={})
        for batch in (1, 16, 32):
            for runner in manifest['runners']:
                for block in range(2):
                    name = f'b{batch}-{runner}-{block}.json'
                    evidence.write(out / name, sample(batch))
                    manifest['hashes'][name] = evidence.sha(out / name)
                    manifest['observations'].append(dict(cell=f'b{batch}-t1', runner=runner, path=name))
        return manifest

    def replay(self, out, manifest):
        evidence.write(out / 'manifest.json', manifest)
        return evidence.replay(out, evidence.sha(out / 'manifest.json'))

    def test_rejects_incomplete_or_nonfinite_observations(self):
        evidence.validate_sample(sample(), 1, 8, 2)
        for field, value in [('success', False), ('completed_calls', 15), ('completed_vectors', 15),
                             ('threads', 2), ('block_elapsed_ms', [0, 1]),
                             ('block_elapsed_ms', [float('nan'), 1]), ('block_elapsed_ms', [1]),
                             ('cold_session_ms', float('inf'))]:
            corrupt = sample()
            corrupt[field] = value
            with self.assertRaises(ValueError):
                evidence.validate_sample(corrupt, 1, 8, 2)

    def test_counts_reject_boolean_float_and_missing_fields(self):
        for field in ('batch', 'threads', 'iterations', 'warmups', 'completed_calls', 'completed_vectors'):
            for value in (True, None, float(sample()[field]), str(sample()[field]), -1, 0):
                with self.subTest(field=field, value=value):
                    corrupt = sample(); corrupt[field] = value
                    with self.assertRaises(ValueError): evidence.validate_sample(corrupt, 1, 8, 2)

    def test_malformed_and_nonfinite_time_series(self):
        for value in (None, True, '1', [], [True, 1], [-1, 1], [float('inf'), 1]):
            corrupt = sample(); corrupt['block_elapsed_ms'] = value
            with self.subTest(value=value), self.assertRaises(ValueError):
                evidence.validate_sample(corrupt, 1, 8, 2)
        for value in (True, None, 0, -1, '1', float('nan')):
            corrupt = sample(); corrupt['cold_session_ms'] = value
            with self.subTest(cold=value), self.assertRaises(ValueError):
                evidence.validate_sample(corrupt, 1, 8, 2)
        with self.assertRaises(ValueError): evidence.validate_sample([], 1, 8, 2)

    def test_process_failure_stops_meter_and_cannot_emit_success(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp); meter = Mock(boundary='cpu package')
            meter.stop.return_value = (5.0, 3)
            with patch.object(evidence, 'ComponentEnergySampler', return_value=meter), \
                    patch.object(evidence, 'command', side_effect=ValueError('child failed')):
                with self.assertRaisesRegex(ValueError, 'child failed'):
                    evidence.sample_run(['test'], out, 'run', 1, 8, 2, energy=True)
            meter.stop.assert_called_once()
            self.assertFalse((out / 'run.energy.json').exists())

    def test_energy_denominator_includes_cold_and_warmup_calls(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp); evidence.write(out / 'run.stdout', sample(16))
            meter = Mock(boundary='cpu package'); meter.stop.return_value = (5.0, 3)
            with patch.object(evidence, 'ComponentEnergySampler', return_value=meter), \
                    patch.object(evidence, 'command', return_value=2.0):
                evidence.sample_run(['test'], out, 'run', 16, 8, 2, energy=True)
            result = json.loads((out / 'run.energy.json').read_text())
            self.assertEqual(result['completed_verified_vectors'], 25 * 16)
            self.assertEqual(result['joules_per_verified_vector'], 5 / 400)
            self.assertEqual(result['average_component_watts'], 2.5)
            self.assertFalse(result['claim_authorized'])
            self.assertFalse(result['physical_system_energy_measured'])

    def test_invalid_meter_values_remain_unavailable(self):
        for joules in (0, -1, True, float('nan'), float('inf')):
            with self.subTest(joules=joules), tempfile.TemporaryDirectory() as tmp:
                out = pathlib.Path(tmp); evidence.write(out / 'run.stdout', sample())
                meter = Mock(boundary='cpu package'); meter.stop.return_value = (joules, 3)
                with patch.object(evidence, 'ComponentEnergySampler', return_value=meter), \
                        patch.object(evidence, 'command', return_value=2.0):
                    evidence.sample_run(['test'], out, 'run', 1, 8, 2, energy=True)
                result = json.loads((out / 'run.energy.json').read_text())
                self.assertFalse(result['available'])
                self.assertIsNone(result['hardware_measured_joules'])
                self.assertIsNone(result['average_component_watts'])

    def test_meter_read_failure_is_explicit(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp); evidence.write(out / 'run.stdout', sample())
            meter = Mock(boundary='cpu package'); meter.stop.side_effect = OSError('counter disappeared')
            with patch.object(evidence, 'ComponentEnergySampler', return_value=meter), \
                    patch.object(evidence, 'command', return_value=2.0):
                evidence.sample_run(['test'], out, 'run', 1, 8, 2, energy=True)
            result = json.loads((out / 'run.energy.json').read_text())
            self.assertFalse(result['available'])
            self.assertIn('counter disappeared', result['reason'])

    def test_replay_rejects_wrong_digest_and_invalid_protocol(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp); manifest = self.bundle(out)
            self.replay(out, manifest)
            with self.assertRaisesRegex(ValueError, 'manifest hash mismatch'):
                evidence.replay(out, '0' * 64)
            for key, value in [('runners', ['head', 'head', 'direct']), ('runners', ['head']),
                               ('rounds', 0), ('rounds', True), ('blocks', 21), ('iterations', 0),
                               ('latency_claim_authorized', True), ('energy_claim_authorized', True)]:
                corrupt = copy.deepcopy(manifest); corrupt[key] = value
                with self.subTest(key=key, value=value), self.assertRaises(ValueError): self.replay(out, corrupt)

    def test_replay_rejects_unknown_duplicate_and_unhashed_observations(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp); manifest = self.bundle(out)
            for field, value in [('cell', 'b16-t2'), ('runner', 'unknown'), ('path', 'missing.json'), ('path', [])]:
                corrupt = copy.deepcopy(manifest); corrupt['observations'][0][field] = value
                with self.subTest(field=field), self.assertRaises(ValueError): self.replay(out, corrupt)
            corrupt = copy.deepcopy(manifest); corrupt['observations'][1] = corrupt['observations'][0]
            with self.assertRaises(ValueError): self.replay(out, corrupt)
            corrupt = copy.deepcopy(manifest); corrupt['observations'].append(corrupt['observations'][0])
            with self.assertRaises(ValueError): self.replay(out, corrupt)

    def test_replay_rejects_unsafe_paths_and_symlinks(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp); out = root / 'bundle'; out.mkdir()
            manifest = self.bundle(out)
            target = out / manifest['observations'][0]['path']
            for name in ('../outside.json', str(target.resolve()), 'sub/../' + target.name):
                corrupt = copy.deepcopy(manifest); corrupt['hashes'][name] = evidence.sha(target)
                with self.subTest(name=name), self.assertRaises(ValueError): self.replay(out, corrupt)
            link = out / 'linked.json'; link.symlink_to(target)
            corrupt = copy.deepcopy(manifest); corrupt['hashes'][link.name] = evidence.sha(target)
            with self.assertRaises(ValueError): self.replay(out, corrupt)

    def test_no_meter_still_executes_but_never_invents_joules(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp)
            evidence.write(out / 'run.stdout', sample())
            with patch.object(evidence, 'ComponentEnergySampler', side_effect=RuntimeError('no meter')), \
                    patch.object(evidence, 'command', return_value=1.0) as command:
                evidence.sample_run(['test'], out, 'run', 1, 8, 2, energy=True)
                command.assert_called_once()
            energy = json.loads((out / 'run.energy.json').read_text())
            self.assertFalse(energy['available'])
            self.assertIsNone(energy['hardware_measured_joules'])
            self.assertIsNone(energy['average_component_watts'])
            self.assertFalse(energy['claim_authorized'])

    def test_replay_rejects_tampering_and_missing_samples(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp)
            manifest = dict(schema='shorthand.generated_infer.bundle.v1', runners=['head', 'direct'],
                            rounds=1, iterations=8, blocks=2, workload='test fixture', observations=[], hashes={})
            for batch in (1, 16, 32):
                for runner in manifest['runners']:
                    for block in range(2):
                        name = f'b{batch}-{runner}-{block}.json'
                        evidence.write(out / name, sample(batch))
                        manifest['hashes'][name] = evidence.sha(out / name)
                        manifest['observations'].append(dict(cell=f'b{batch}-t1', runner=runner, path=name))
            evidence.write(out / 'manifest.json', manifest)
            result = evidence.replay(out, evidence.sha(out / 'manifest.json'))
            self.assertEqual(len(result['rows']), 3)
            self.assertFalse(result['energy_claim_authorized'])
            broken = copy.deepcopy(manifest)
            broken['observations'].pop()
            evidence.write(out / 'manifest.json', broken)
            with self.assertRaises(ValueError):
                evidence.replay(out, evidence.sha(out / 'manifest.json'))
            evidence.write(out / 'manifest.json', manifest)
            (out / manifest['observations'][0]['path']).write_text('{}')
            with self.assertRaises(ValueError):
                evidence.replay(out, evidence.sha(out / 'manifest.json'))


if __name__ == '__main__':
    unittest.main()
