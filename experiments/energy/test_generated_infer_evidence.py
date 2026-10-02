#!/usr/bin/env python3
import copy
import json
import pathlib
import tempfile
import unittest
from unittest.mock import patch
import generated_infer_evidence as evidence


def sample(batch=1):
    return dict(schema='shorthand.generated_infer.sample.v1', success=True, batch=batch,
                threads=1, iterations=8, warmups=8, completed_calls=16,
                completed_vectors=16*batch, block_elapsed_ms=[1.0, 1.1], cold_session_ms=2.0)


class GeneratedEvidenceTest(unittest.TestCase):
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
