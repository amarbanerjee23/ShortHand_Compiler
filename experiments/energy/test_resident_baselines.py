"""Synthetic evidence tests; these do not measure any implementation."""
import collections
import copy
import itertools
import pathlib
import tempfile
import unittest

import resident_baselines as baseline


class ResidentTests(unittest.TestCase):
    def test_balanced_design(self):
        for count in (6, 12):
            sequence = baseline.orders(108, count)
            self.assertEqual(collections.Counter(map(tuple, sequence)),
                             collections.Counter({p: count // 6 for p in itertools.permutations(baseline.RUNNERS)}))
            self.assertEqual(sequence, baseline.orders(108, count))
        for count in (0, 5, 7, True):
            with self.assertRaises(ValueError): baseline.orders(108, count)

    def test_top_k_scores_and_ties(self):
        report = dict(scores=[0.] * 17970, predictions=[0] * 1797, top_k=[0, 1, 2] * 1797)
        baseline.validate_classification(report)
        for field, index, value in [('top_k', 1, 2), ('predictions', 0, 1),
                                    ('scores', 0, float('nan')), ('top_k', 0, True)]:
            bad = copy.deepcopy(report); bad[field][index] = value
            with self.subTest(field=field), self.assertRaises(ValueError): baseline.validate_classification(bad)
        report['top_k'].pop()
        with self.assertRaises(ValueError): baseline.validate_classification(report)

    def test_resource_windows_and_failed_process(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp); path = root / 'native.resources.csv'
            path.write_text('0.01,0.02,1024\n')
            self.assertEqual(baseline.resources(path), dict(process_cpu_ms=30, maximum_rss_bytes=1048576))
            baseline.campaign.write(root / 'native.json', dict(returncode=1, elapsed_ms=30, completed=10782))
            with self.assertRaises(ValueError): baseline.checked_process(root, 'native')
            for invalid in ('NaN,0,1024', '1,-1,1024', '0,0,0', '1,1'):
                path.write_text(invalid)
                with self.assertRaises(ValueError): baseline.resources(path)

    def test_manifest_rejects_changed_bytes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp); raw = root / 'raw.json'
            raw.write_text('{}\n')
            baseline.campaign.write(root / 'manifest.json', dict(schema=baseline.SCHEMA, success=True,
                hashes={'raw.json': baseline.campaign.sha(raw)}))
            digest = baseline.campaign.sha(root / 'manifest.json')
            raw.write_text('{"changed":true}\n')
            with self.assertRaisesRegex(ValueError, 'artifact changed'):
                baseline.analyze(root, digest)
            with self.assertRaisesRegex(ValueError, 'manifest digest'):
                baseline.analyze(root, '0' * 64)


if __name__ == '__main__':
    unittest.main()
