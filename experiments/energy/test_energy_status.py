#!/usr/bin/env python3
import json
import pathlib
import tempfile
import unittest

import energy_status


class EnergyStatusTest(unittest.TestCase):
    def test_hosted_capture_never_invents_joules(self):
        with tempfile.TemporaryDirectory() as directory:
            value = energy_status.status(pathlib.Path(directory))
        self.assertEqual(value['schema'], 'shorthand.energy.availability.v1')
        self.assertEqual(value['mode'], 'continuous_ci_evidence')
        self.assertFalse(value['physical_energy_measured'])
        self.assertFalse(value['component_energy_measured'])
        self.assertIsNone(value['energy_savings_percent'])
        self.assertIsNone(value['joules_per_completed_task'])
        self.assertFalse(value['claim_authorized'])
        self.assertIn('elapsed_time', value['disallowed_substitutes'])
        self.assertIn('unmatched_reference_coefficients', value['disallowed_substitutes'])
        self.assertNotIn('rapl', value['disallowed_substitutes'])

    def test_cli_shape_remains_backward_safe(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'energy-status.json'
            value = energy_status.status(pathlib.Path(directory))
            path.write_text(json.dumps(value) + '\n')
            observed = json.loads(path.read_text())
            for key in ('physical_energy_measured', 'energy_savings_percent',
                        'joules_per_completed_task', 'source', 'reason'):
                self.assertIn(key, observed)
            self.assertFalse(observed['physical_energy_measured'])

    def test_supported_classes_are_explicit(self):
        with tempfile.TemporaryDirectory() as directory:
            value = energy_status.status(pathlib.Path(directory))
        self.assertEqual(set(value['supported_evidence_classes']), {'E0', 'E1', 'E2', 'E3'})
        self.assertIsNone(value['evidence_class'])


if __name__ == '__main__':
    unittest.main()
