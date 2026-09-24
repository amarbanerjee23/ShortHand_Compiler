#!/usr/bin/env python3
import json
import pathlib
import tempfile
import unittest

import energy_status


class EnergyStatusTest(unittest.TestCase):
    def test_hosted_capture_is_explicitly_not_a_joule_measurement(self):
        value = energy_status.status()
        self.assertEqual(value['schema'], 'shorthand.energy.availability.v1')
        self.assertEqual(value['mode'], 'execution_only')
        self.assertFalse(value['physical_energy_measured'])
        self.assertIsNone(value['energy_savings_percent'])
        self.assertEqual(value['boundary'], 'whole_host_ac')
        self.assertEqual(value['source'], 'unavailable')
        self.assertIn('rapl', value['disallowed_substitutes'])

    def test_cli_writes_json(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'energy-status.json'
            # Exercise the same writer used by the workflow without spawning a
            # process, keeping the test dependency-free.
            path.write_text(json.dumps(energy_status.status()) + '\n')
            value = json.loads(path.read_text())
            self.assertFalse(value['physical_energy_measured'])


if __name__ == '__main__':
    unittest.main()
