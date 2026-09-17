"""Real native producer/signature/replay integration; fixtures are not field evidence."""
import copy
import csv
import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import subprocess
import sys

TOOL, WORK, ROOT, MEASURE = sys.argv[1], Path(sys.argv[2]), Path(sys.argv[3]), sys.argv[4]
PILOT = WORK / 'pilot'
PILOT.mkdir()
AS_OF = '2026-09-09'
SCHEMA = 'shorthand.c3eco.independent_pilot.v1'
BUNDLE_SCHEMA = 'shorthand.c3eco.auditor_bundle.v1'
FLAGS = ('official_certification_granted', 'level_claim_permitted', 'comparative_energy_claim', 'production_claim')
CASES = 0


def run(*args, code=0, diagnostic=None):
    result = subprocess.run([str(a) for a in args], capture_output=True, text=True, timeout=60)
    assert result.returncode == code, (args, result.returncode, result.stdout, result.stderr)
    assert not any(x in result.stderr for x in ('AddressSanitizer', 'LeakSanitizer', 'runtime error:')), result.stderr
    if diagnostic:
        assert diagnostic in result.stderr, (diagnostic, result.stderr)
    return result.stdout


def dump(path, value):
    path.write_text(json.dumps(value, sort_keys=True, separators=(',', ':')) + '\n')


def read(path):
    return json.loads(path.read_text())


def sha(data):
    return hashlib.sha256(data).hexdigest()


def schema_check(value, name):
    # Check native receipts and signed fixtures against the shipped contracts.
    def check(v, spec):
        if 'const' in spec:
            assert type(v) is type(spec['const']) and v == spec['const']
        if 'enum' in spec:
            assert v in spec['enum']
        kind = spec.get('type')
        if kind == 'object':
            assert isinstance(v, dict)
            assert set(spec.get('required', [])) <= v.keys()
            if spec.get('additionalProperties') is False:
                assert v.keys() <= spec['properties'].keys()
            for key, child in v.items():
                check(child, spec['properties'][key])
        elif kind == 'array':
            assert isinstance(v, list)
            assert spec.get('minItems', 0) <= len(v) <= spec.get('maxItems', math.inf)
            if spec.get('uniqueItems'):
                assert len({json.dumps(child, sort_keys=True) for child in v}) == len(v)
            for child in v:
                check(child, spec['items'])
        elif kind == 'string':
            assert isinstance(v, str)
            assert spec.get('minLength', 0) <= len(v) <= spec.get('maxLength', math.inf)
            if 'pattern' in spec:
                assert re.search(spec['pattern'], v)
            if spec.get('format') == 'date':
                assert datetime.date.fromisoformat(v).isoformat() == v
        elif kind == 'boolean':
            assert type(v) is bool
        elif kind in ('number', 'integer'):
            assert type(v) in (int, float) and math.isfinite(v)
            if kind == 'integer':
                assert int(v) == v
            assert spec.get('minimum', -math.inf) <= v <= spec.get('maximum', math.inf)
            if 'exclusiveMinimum' in spec:
                assert v > spec['exclusiveMinimum']
    check(value, read(ROOT / 'schemas/c3eco' / (name + '_v1.schema.json')))


def inventory(root, excluded):
    return [{'path': p.relative_to(root).as_posix(), 'bytes': p.stat().st_size, 'sha256': sha(p.read_bytes())}
            for p in sorted(root.rglob('*'), key=lambda p: p.relative_to(root).as_posix())
            if p.is_file() and not (p.parent == root and p.name in excluded)]


def sign(root, document, key, schema):
    payload = WORK / 'pilot-sign-payload'
    signature = WORK / 'pilot-signature.bin'
    payload.write_bytes(schema.encode() + b'\n' + (root / (document + '.json')).read_bytes())
    run('openssl', 'pkeyutl', '-sign', '-inkey', key, '-rawin', '-in', payload, '-out', signature)
    (root / (document + '.sig')).write_text(signature.read_bytes().hex())


def reseal_bundle(bundle, key):
    manifest = read(bundle / 'manifest.json')
    manifest['artifacts'] = inventory(bundle, {'manifest.json', 'manifest.sig'})
    dump(bundle / 'manifest.json', manifest)
    sign(bundle, 'manifest', key, BUNDLE_SCHEMA)


keys = {}
for role in ('reference', 'repeat', 'reviewer'):
    key, public = WORK / (role + '.pem'), WORK / (role + '-public.pem')
    run('openssl', 'genpkey', '-algorithm', 'ED25519', '-out', key)
    run('openssl', 'pkey', '-in', key, '-pubout', '-out', public)
    keys[role] = key

base = WORK / 'bundle' / 'candidate'
revision = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
expected_output = (base / 'profile.json').read_bytes()  # Actual compiler-produced profile output.
runs = []
with (WORK / 'generated/measurement.tsv').open() as stream:
    template = list(csv.DictReader(stream, delimiter='\t'))
for role in ('reference', 'repeat'):
    for index in range(3):
        name = role + '-' + str(index)
        candidate = WORK / ('pilot-candidate-' + name)
        shutil.copytree(base, candidate)
        rows = copy.deepcopy(template)
        for row in rows:
            row['measured_at'] = '2026-09-01T10:0' + str(index) + ':00Z'
            row['instrument_id'] = role + '-' + row['instrument_id']
            row['raw_energy_j'] = str(float(row['raw_energy_j']) * (1 + index * .01 + (role == 'repeat') * .01))
        tsv = WORK / ('pilot-' + name + '.tsv')
        with tsv.open('w', newline='') as stream:
            writer = csv.DictWriter(stream, fieldnames=list(rows[0]), delimiter='\t', lineterminator='\n')
            writer.writeheader()
            writer.writerows(rows)
        run(MEASURE, tsv, WORK / ('pilot-' + name + '.csv'), candidate / 'measurement.json')
        env = json.dumps({'fixture': True, 'host': role}, sort_keys=True).encode()
        (candidate / 'evidence/pilot-environment.json').write_bytes(env)
        (candidate / 'evidence/pilot-output.json').write_bytes(expected_output)
        dump(candidate / 'evidence/pilot-run.json', {
            'schema': 'shorthand.c3eco.pilot_run.v1', 'run_id': name, 'organization': role + '-org',
            'executed_on': '2026-09-01', 'environment_sha256': sha(env),
            'environment_ref': 'evidence/pilot-environment.json', 'result_sha256': sha(expected_output),
            'result_ref': 'evidence/pilot-output.json', 'revision': revision, 'completed_units': 100,
        })
        policy = read(WORK / 'bundle/policy.json')
        policy['bundle_id'] = name
        policy_file = WORK / ('pilot-policy-' + name + '.json')
        dump(policy_file, policy)
        run(TOOL, 'pack', candidate, policy_file, keys[role], PILOT / name)
        runs.append({'role': role, 'bundle_path': name})

operations = {
    'schema': 'shorthand.c3eco.pilot_operations.v1', 'custodian': 'fixture-custodian',
    'observed_on': AS_OF, 'retain_until': '2029-08-27', 'next_surveillance_on': '2027-03-09',
    'immutable_storage_enabled': True, 'encryption_enabled': True, 'access_review_passed': True,
    'open_major_findings': 0, 'unreviewed_changes': 0,
}
(PILOT / 'operations').mkdir()
for kind in ('storage_evidence', 'access_evidence', 'surveillance_evidence', 'independence_evidence', 'draft_review', 'restore_source', 'restore_result'):
    ref = 'operations/' + kind + '.json'
    operations[kind + '_ref'] = ref
    dump(PILOT / ref, {'fixture': True, 'record': 'restore' if kind.startswith('restore_') else kind})
dump(PILOT / 'operations.json', operations)
reviewer_der = subprocess.check_output(['openssl', 'pkey', '-pubin', '-in', str(WORK / 'reviewer-public.pem'), '-outform', 'DER'])
review = {
    'schema': SCHEMA, 'pilot_id': 'pilot-101', 'scope': 'linux-x64-cpu-v1',
    'rules_id': 'shorthand.c3eco.rules.v0.6+v0.7-20260718.v1', 'reviewer_key_id': sha(reviewer_der[-32:]),
    'reviewed_on': AS_OF, 'valid_until': '2027-03-08', 'runs': runs, **dict.fromkeys(FLAGS, False),
}
trust = {
    'schema': 'shorthand.c3eco.pilot_trust.v1', 'pilot_id': 'pilot-101',
    'expected_profile_sha256': sha((base / 'profile.json').read_bytes()),
    'expected_metadata_sha256': sha((base / 'metadata.tsv').read_bytes()),
    'expected_result_sha256': sha(expected_output), 'expected_revision': revision,
    'min_repetitions': 3, 'max_difference_percent': 15, 'max_uncertainty_percent': 10,
}
for role in keys:
    trust[role + '_organization'] = role + '-org'
    trust[role + '_key'] = role + '-public.pem'
TRUST = WORK / 'pilot-trust.json'
dump(TRUST, trust)


def seal(root, value=None):
    value = copy.deepcopy(value if value is not None else read(root / 'review.json'))
    value['artifacts'] = inventory(root, {'review.json', 'review.sig'})
    dump(root / 'review.json', value)
    sign(root, 'review', keys['reviewer'], SCHEMA)


def verify(root=PILOT, policy=TRUST, date=AS_OF, code=0, diagnostic=None, blocker=None):
    global CASES
    CASES += 1
    text = run(TOOL, 'verify-pilot', root, policy, date, code=code, diagnostic=diagnostic)
    if code == 2:
        assert not text, 'invalid evidence must not produce a success receipt'
        return None
    result = json.loads(text)
    schema_check(result, 'pilot_verification')
    assert not any(result[k] for k in (*FLAGS, 'organizational_independence_authenticated',
                                     'physical_measurements_independently_verified', 'storage_retention_independently_verified'))
    assert result['all_assessments_replayed'] and result['signature_verified']
    assert result['decision'] == ('candidate_reproduction_consistent' if code == 0 else 'blocked_by_pilot_evidence')
    if blocker:
        assert blocker in result['blockers'], result
    return result


def clone():
    path = WORK / ('pilot-case-' + str(CASES))
    shutil.copytree(PILOT, path)
    return path


seal(PILOT, review)
schema_check(read(PILOT / 'review.json'), 'independent_pilot')
schema_check(trust, 'pilot_trust')
schema_check(operations, 'pilot_operations')
schema_check(read(PILOT / 'reference-0/candidate/evidence/pilot-run.json'), 'pilot_run')
result = verify()
assert result['reference_runs'] == result['repeat_runs'] == 3
assert abs(result['reference_j_per_unit'] - 65448) < 1e-6
assert abs(result['repeat_j_per_unit'] - 66096) < 1e-6
assert result == verify(), 'pilot verification must be deterministic'
for field, value, message in [
    ('repeat_organization', 'reference-org', 'organizations must be distinct'),
    ('reviewer_key', 'reference-public.pem', 'signing keys must be distinct'),
    ('expected_result_sha256', '0' * 64, 'output or revision mismatch'),
    ('expected_metadata_sha256', '0' * 64, 'workload or functional boundary differs'),
    ('expected_revision', '0' * 40, 'output or revision mismatch'),
    ('min_repetitions', 4, 'insufficient independent repetitions'),
    ('min_repetitions', 2, 'numeric bound'),
]:
    changed = dict(trust, **{field: value})
    policy = WORK / ('changed-trust-' + str(CASES) + '.json')
    dump(policy, changed)
    verify(policy=policy, code=2, diagnostic=message)
for field, value, blocker in [('max_difference_percent', 1, 'repeatability_difference_exceeds_policy'),
                             ('max_uncertainty_percent', 1, 'measurement_uncertainty_exceeds_policy')]:
    policy = WORK / ('changed-trust-' + str(CASES) + '.json')
    dump(policy, dict(trust, **{field: value}))
    verify(policy=policy, code=3, blocker=blocker)
for mutation in ('tamper', 'missing', 'extra', 'signature'):
    path = clone()
    target = path / 'operations/storage_evidence.json'
    if mutation == 'tamper': target.write_text('tampered')
    elif mutation == 'missing': target.unlink()
    elif mutation == 'extra': (path / 'unexpected').write_text('extra')
    else: (path / 'review.sig').write_text('0' * 128)
    verify(path, code=2, diagnostic='signature verification failed' if mutation == 'signature' else 'inventory or digest mismatch')
for field in FLAGS:
    path = clone()
    value = read(path / 'review.json')
    value[field] = True
    seal(path, value)
    verify(path, code=2, diagnostic='unsupported pilot claim')
for field, value, blocker in [
    ('immutable_storage_enabled', False, 'organizational_storage_or_access_control_failed'),
    ('encryption_enabled', False, 'organizational_storage_or_access_control_failed'),
    ('access_review_passed', False, 'organizational_storage_or_access_control_failed'),
    ('retain_until', '2027-03-08', 'organizational_retention_too_short'),
    ('open_major_findings', 1, 'organizational_review_has_open_findings'),
    ('unreviewed_changes', 1, 'organizational_review_has_open_findings'),
]:
    path = clone()
    dump(path / 'operations.json', dict(operations, **{field: value}))
    seal(path)
    verify(path, code=3, blocker=blocker)
path = clone()
(path / 'operations/restore_result.json').write_text('corrupt restore')
seal(path)
verify(path, code=3, blocker='organizational_restore_digest_mismatch')
verify(date='2027-03-09', code=3, blocker='organizational_surveillance_due')
verify(date='2027-03-09', code=3, blocker='pilot_review_expired')
verify(date='2026-09-08', code=2, diagnostic='invalid pilot review dates')
for field, value, message in [('completed_units', 0, 'numeric bound'),
                              ('result_ref', '../profile.json', 'invalid identifier'),
                              ('executed_on', '2026-09-10', 'run is from the future'),
                              ('environment_sha256', '0' * 64, 'artifact digest mismatch')]:
    path = clone()
    bundle = path / 'repeat-0'
    attestation = read(bundle / 'candidate/evidence/pilot-run.json')
    attestation[field] = value
    dump(bundle / 'candidate/evidence/pilot-run.json', attestation)
    reseal_bundle(bundle, keys['repeat'])
    seal(path)
    verify(path, code=2, diagnostic=message)
path = clone()
value = read(path / 'review.json')
value['runs'][1]['bundle_path'] = value['runs'][0]['bundle_path']
seal(path, value)
verify(path, code=2, diagnostic='duplicate pilot bundle path')
path = clone()
(path / 'repeat-0/candidate/evidence/pilot-output.json').write_text('changed output')
reseal_bundle(path / 'repeat-0', keys['repeat'])
seal(path)
verify(path, code=2, diagnostic='artifact digest mismatch')

# The reviewer cannot turn duplicated or non-independent inputs into repeats,
# even after all affected inner bundles and the outer review are signed again.
for mutation, message in [('workbook', 'reused measurement workbook'),
                          ('sample', 'reused instrument sample'),
                          ('environment', 'environments overlap')]:
    path = clone()
    name = 'reference-1' if mutation != 'environment' else 'repeat-0'
    bundle = path / name
    candidate = WORK / ('repacked-candidate-' + str(CASES))
    shutil.copytree(bundle / 'candidate', candidate)
    if mutation == 'workbook':
        shutil.copyfile(path / 'reference-0/candidate/measurement.json', candidate / 'measurement.json')
    elif mutation == 'sample':
        workbook = read(candidate / 'measurement.json')
        for record in workbook['records']:
            record['measured_at'] = '2026-09-01T10:00:00Z'
        dump(candidate / 'measurement.json', workbook)
    else:
        environment = (path / 'reference-0/candidate/evidence/pilot-environment.json').read_bytes()
        (candidate / 'evidence/pilot-environment.json').write_bytes(environment)
        attestation = read(candidate / 'evidence/pilot-run.json')
        attestation['environment_sha256'] = sha(environment)
        dump(candidate / 'evidence/pilot-run.json', attestation)
    policy = WORK / ('repacked-policy-' + str(CASES) + '.json')
    shutil.copyfile(bundle / 'policy.json', policy)
    shutil.rmtree(bundle)
    run(TOOL, 'pack', candidate, policy, keys[name.split('-')[0]], bundle)
    seal(path)
    verify(path, code=2, diagnostic=message)
path = clone()
reseal_bundle(path / 'repeat-0', keys['reference'])
seal(path)
verify(path, code=2, diagnostic='signature verification failed')
path = clone()
(path / 'operations/independence_evidence.json').unlink()
seal(path)
verify(path, code=2, diagnostic='missing retained organizational evidence')
path = clone()
dump(path / 'operations.json', dict(operations, restore_result_ref=operations['restore_source_ref']))
seal(path)
verify(path, code=2, diagnostic='separate artifacts')
for field, value in [('unexpected', True), ('schema', 'unsupported.v2')]:
    path = clone()
    value = dict(read(path / 'review.json'), **{field: value})
    seal(path, value)
    verify(path, code=2)
path = clone()
dump(path / 'embedded-trust.json', trust)
verify(path, policy=path / 'embedded-trust.json', code=2, diagnostic='outside evidence inputs')

# Opposing outliers preserve the group mean but must fail repeatability.
path = clone()
for index, factor in ((0, .50), (2, 1.52)):
    bundle = path / ('reference-' + str(index))
    candidate = WORK / ('unstable-candidate-' + str(index))
    shutil.copytree(bundle / 'candidate', candidate)
    rows = copy.deepcopy(template)
    for row in rows:
        row['measured_at'] = '2026-09-01T10:0' + str(index) + ':00Z'
        row['instrument_id'] = 'reference-' + row['instrument_id']
        row['raw_energy_j'] = str(float(row['raw_energy_j']) * factor)
    tsv = WORK / ('unstable-' + str(index) + '.tsv')
    with tsv.open('w', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]), delimiter='\t', lineterminator='\n')
        writer.writeheader()
        writer.writerows(rows)
    run(MEASURE, tsv, WORK / ('unstable-' + str(index) + '.csv'), candidate / 'measurement.json')
    policy = WORK / ('unstable-policy-' + str(index) + '.json')
    shutil.copyfile(bundle / 'policy.json', policy)
    shutil.rmtree(bundle)
    run(TOOL, 'pack', candidate, policy, keys['reference'], bundle)
seal(path)
unstable = verify(path, code=3, blocker='within_group_variation_exceeds_policy')
assert abs(unstable['reference_j_per_unit'] - result['reference_j_per_unit']) < 1e-6
assert 'repeatability_difference_exceeds_policy' not in unstable['blockers']
print(f'PASS PR101 independent pilot integration cases={CASES}; signed repeats, replay, retention, restore and claim safety')
