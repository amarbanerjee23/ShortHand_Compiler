#!/usr/bin/env python3
"""Replay real retained evidence; reject deliberately corrupted derivatives."""
import argparse
import copy
import json
import pathlib
import unittest
from unittest import mock

import benchmark

BUNDLE = None


class ReplayTest(unittest.TestCase):
    def setUp(self):
        self.manifest = json.loads((BUNDLE/'manifest.json').read_text())
        self.digest = benchmark.sha(BUNDLE/'manifest.json')

    def test_complete_bundle_replays(self):
        report = benchmark.replay(BUNDLE, self.digest)
        self.assertTrue(report['success'])
        self.assertEqual(len(report['rows']),3)
        self.assertFalse(report['energy_claim_authorized'])

    def reject_manifest(self, mutate):
        # Read-only overlay: mutate the decoded manifest, while real artifact
        # bytes and expected root digest remain unchanged. No raw data edited.
        manifest = copy.deepcopy(self.manifest)
        mutate(manifest)
        real_loads = json.loads
        def loads(text):
            data = real_loads(text)
            return manifest if isinstance(data,dict) and data.get('schema')=='shorthand.aot.benchmark.v1' else data
        with mock.patch.object(benchmark.json,'loads',side_effect=loads):
            with mock.patch.object(benchmark,'write') as write, self.assertRaises(ValueError):
                benchmark.replay(BUNDLE,self.digest)
            write.assert_not_called()

    def test_missing_duplicate_misordered_and_unhashed_samples(self):
        for mutate in [lambda m:m['observations'].pop(),
                       lambda m:m['observations'][1].update(path=m['observations'][0]['path']),
                       lambda m:m['observations'][0].update(block=True),
                       lambda m:m['observations'][0].update(runner='ort_all' if m['observations'][0]['runner']=='aot' else 'aot'),
                       lambda m:m['hashes'].pop(m['observations'][0]['path']),
                       lambda m:m['hashes'].pop('b1-t1/artifact/model.o'),
                       lambda m:m['hashes'].update({'../../escape':'0'*64}),
                       lambda m:m['energy'].pop(),
                       lambda m:m['energy'][1].update(path=m['energy'][0]['path']),
                       lambda m:m.update(latency_claim_authorized=True),
                       lambda m:m['cells'][0].update(workspace_bytes=0),
                       lambda m:m['cells'][0].update(object_bytes=1),
                       lambda m:m['cells'][0].update(executable_bytes={})]:
            with self.subTest(mutate=mutate):
                self.reject_manifest(mutate)

    def test_energy_status_rejects_invented_or_incomplete_measurements(self):
        # Synthetic validator input is never written into the retained bundle.
        row=self.manifest['energy'][0]
        e=json.loads((BUNDLE/row['path']).read_text())
        benchmark.validate_energy(e,row['batch'],self.manifest['iterations'])
        for change in [dict(available=True,samples=1),dict(available=False,hardware_measured_joules=1),
                       dict(verified_vectors=0),dict(process_elapsed_seconds=float('nan')),
                       dict(claim_authorized=True),dict(physical_system_energy_measured=True)]:
            with self.subTest(change=change),self.assertRaises(ValueError):
                benchmark.validate_energy(dict(e,**change),row['batch'],self.manifest['iterations'])

    def test_hash_mismatch(self):
        with self.assertRaisesRegex(ValueError,'digest mismatch'):
            benchmark.replay(BUNDLE,'0'*64)
        self.reject_manifest(lambda m:m['hashes'].update({'b1-t1/aot':'0'*64}))

    def test_sample_contract_rejects_wrong_counts_nonfinite_and_false_success(self):
        row = self.manifest['observations'][0]
        original = json.loads((BUNDLE/row['path']).read_text())
        for key,value in [('iterations',True),('completed_calls',0),('warmups',0),
                          ('elapsed_ms',float('nan')),('cold_ms',0),('success',False),('batch',16)]:
            bad = dict(original,**{key:value})
            with self.subTest(key=key),self.assertRaises(ValueError):
                benchmark.validate_sample(bad,row['runner'],row['batch'],self.manifest['iterations'])


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--bundle',type=pathlib.Path,required=True)
    args,remaining=parser.parse_known_args()
    BUNDLE=args.bundle.resolve()
    unittest.main(argv=[__file__,*remaining])
