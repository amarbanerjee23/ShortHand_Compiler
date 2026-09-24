"""Synthetic timings exercise report validation only, never performance evidence."""
import copy
import unittest
from profile_report import markdown, validate


def fixture():
    return dict(schema='shorthand.ai.application.profile.v1', success=True, diagnostic_only=True,
                production_claim=False, comparative_energy_claim=False, official_certification_granted=False,
                measured_energy_available=False, latency_claim_eligible=False,
                rows=3, repetitions=2, batch_size=2, threads=1, accuracy=.9, minimum_accuracy=.85,
                trials=[dict(preprocessing_ns=10, prepared_call_ns=60, output_validation_ns=10,
                             postprocessing_ns=20, total_ns=100, completed=6, batches=4, observer_elapsed_ms=.001)])


class ProfileTests(unittest.TestCase):
    def test_partition_and_render(self):
        self.assertEqual(validate(fixture())['total_ns'], 100)
        self.assertIn('60.00%', markdown(fixture(), '0' * 64))
        self.assertIn('no energy', markdown(fixture(), '0' * 64))

    def test_rejects_incomplete_invalid_and_claimed_observations(self):
        for key, value in [('completed', 5), ('batches', 3), ('total_ns', 99),
                           ('preprocessing_ns', -1), ('prepared_call_ns', float('nan')),
                           ('postprocessing_ns', True), ('observer_elapsed_ms', 0)]:
            report = fixture(); report['trials'][0][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate(report)
        for key, value in [('schema', 'shorthand.ai.application.report.v1'), ('success', False),
                           ('comparative_energy_claim', True), ('measured_energy_available', True),
                           ('latency_claim_eligible', True), ('accuracy', .1), ('rows', 0), ('trials', [])]:
            report = fixture(); report[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate(report)

    def test_keeps_all_trials(self):
        report = fixture(); report['trials'].append(copy.deepcopy(report['trials'][0]))
        self.assertEqual(validate(report)['completed'], 12)
        self.assertEqual(validate(report)['total_ns'], 200)


if __name__ == '__main__':
    unittest.main()
