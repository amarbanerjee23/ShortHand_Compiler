#!/usr/bin/env python3
import json
import pathlib
import tempfile
import unittest
import ci_run_evidence as evidence


class RunEvidenceTest(unittest.TestCase):
    def fixture(self, out):
        evidence.initialize(out)
        for claim, name in evidence.REQUIRED.items():
            path = out / name
            path.parent.mkdir(parents=True, exist_ok=True)
            if name.endswith('.json'):
                value = dict(schema=evidence.SCHEMAS[claim], success=True,
                             latency_claim_authorized=False,
                             energy_claim_authorized=False,
                             power_claim_authorized=False,
                             claim_authorized=False)
                if claim == 'resident_component_energy_or_unavailable':
                    value.update(success=True, available=False,
                                 component_energy_measured=False,
                                 evidence_class=None,
                                 reason='test_meter_unavailable',
                                 hardware_measured_joules=None)
                evidence.write(path, value)
            else:
                with path.open('a') as stream:
                    stream.write(evidence.PASS_MARKERS[claim] + '\n')

    def test_missing_or_failed_work_cannot_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp)
            evidence.initialize(out)
            self.assertEqual(evidence.finalize(out, 'success')['status'], 'incomplete')
            self.fixture(out)
            self.assertEqual(evidence.finalize(out, 'failure')['status'], 'incomplete')
            self.assertEqual(evidence.finalize(out, 'cancelled')['status'], 'incomplete')
            self.assertEqual(evidence.finalize(out, 'success')['status'], 'complete')
            (out / 'generated-infer/report.json').write_text('{}')
            self.assertEqual(evidence.finalize(out, 'success')['status'], 'incomplete')

    def test_energy_absence_is_recorded_without_authorizing_claims(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp)
            self.fixture(out)
            path = out / evidence.REQUIRED['resident_component_energy_or_unavailable']
            evidence.write(path, dict(schema=evidence.SCHEMAS['resident_component_energy_or_unavailable'],
                                      success=True, available=False,
                                      component_energy_measured=False,
                                      evidence_class=None,
                                      reason='test_meter_unavailable',
                                      hardware_measured_joules=None,
                                      latency_claim_authorized=False,
                                      energy_claim_authorized=False,
                                      power_claim_authorized=False,
                                      claim_authorized=False))
            result = evidence.finalize(out, 'success')
            self.assertEqual(result['status'], 'complete')
            for key in ('latency', 'energy', 'power'):
                self.assertIs(result[key + '_reduction_claim_authorized'], False)
            self.assertIn('ci-energy/summary.json', result['hashes'])
            self.assertTrue((out / 'EVIDENCE.md').exists())
            path.write_text('{broken')
            self.assertEqual(evidence.finalize(out, 'success')['status'], 'incomplete')


if __name__ == '__main__':
    unittest.main()
