#!/usr/bin/env python3
"""Offline auditor inventory corruption tests; no physical evidence asserted."""
import copy
import json
import pathlib
import tempfile
import unittest
import export_auditor_kit as kit


class AuditorKitTests(unittest.TestCase):
    def test_complete_inventory_and_corruptions(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            artifacts = {}
            for name in (*kit.CORE_ARTIFACTS, 'release_closeout_report.json'):
                path = root / name if name == 'release_closeout_report.json' else root / 'artifacts' / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('synthetic evidence\n')
                artifacts[name] = {'sha256': kit.sha256_file(path), 'bytes': path.stat().st_size}
            manifest = {'schema': kit.SCHEMA, 'artifact_count': len(artifacts), 'artifacts': artifacts}
            index = root / 'SHA256SUMS.json'
            index.write_text(json.dumps(manifest))
            self.assertEqual(kit.verify_kit(root), (True, []))
            mutations = [dict(manifest, artifacts={}), dict(manifest, artifact_count=0)]
            for key in artifacts:
                changed = copy.deepcopy(manifest)
                del changed['artifacts'][key]
                mutations.append(changed)
            changed = copy.deepcopy(manifest)
            changed['artifacts']['../outside'] = {'sha256': 'a'*64, 'bytes': 0}
            mutations.append(changed)
            for changed in mutations:
                index.write_text(json.dumps(changed))
                self.assertFalse(kit.verify_kit(root)[0])
            index.write_text(json.dumps(manifest))
            path = root / 'artifacts' / kit.CORE_ARTIFACTS[0]
            path.write_text('tampered')
            self.assertFalse(kit.verify_kit(root)[0])
            path.unlink()
            path.symlink_to(root / 'release_closeout_report.json')
            self.assertFalse(kit.verify_kit(root)[0])


if __name__ == '__main__':
    unittest.main()
