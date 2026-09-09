"""Native CLI integration tests; cryptographic checks use independent OpenSSL CLI."""
import csv
import datetime
import re
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

TOOL, WORK, ROOT = sys.argv[1], Path(sys.argv[2]), Path(sys.argv[3])
CANDIDATE = WORK / 'candidate'
KEY, PUBLIC, OTHER, OTHER_PUBLIC = [WORK / name for name in ('signer.pem', 'trusted.pem', 'other.pem', 'other-public.pem')]
AS_OF = '2026-09-09'
CASES = 0

def run(*args, code=0, diagnostic=None):
    global CASES
    result = subprocess.run([str(a) for a in args], capture_output=True, text=True, timeout=45)
    assert result.returncode == code, (args, result.returncode, result.stdout, result.stderr)
    assert not any(marker in result.stderr for marker in ('AddressSanitizer', 'LeakSanitizer', 'runtime error:', 'UndefinedBehaviorSanitizer')), result.stderr
    if diagnostic:
        assert diagnostic in result.stderr, (args, diagnostic, result.stderr)
    CASES += 1
    return result.stdout

def dump(path, value):
    path.write_bytes((json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False) + '\n').encode('utf-8'))

def read(path):
    return json.loads(path.read_text())


def schema_check(value, name):
    # These published schemas use this explicit JSON Schema assertion subset.
    # Check the independently stored contracts against native producer outputs.
    schema = read(ROOT / 'schemas/c3eco' / (name + '.schema.json'))
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
            assert spec.get('minItems', 0) <= len(v) <= spec.get('maxItems', float('inf'))
            for child in v:
                check(child, spec['items'])
        elif kind == 'string':
            assert isinstance(v, str) and len(v) >= spec.get('minLength', 0)
            if 'pattern' in spec:
                assert re.search(spec['pattern'], v)
            if spec.get('format') == 'date':
                assert datetime.date.fromisoformat(v).isoformat() == v
        elif kind == 'boolean':
            assert type(v) is bool
        elif kind in ('number', 'integer'):
            assert type(v) in (int, float)
            if kind == 'integer':
                assert int(v) == v
            assert spec.get('minimum', float('-inf')) <= v <= spec.get('maximum', float('inf'))
            if 'exclusiveMinimum' in spec:
                assert v > spec['exclusiveMinimum']
    check(value, schema)

# Gate and criterion ids overlap; use distinct portable paths for their evidence.
gates_path = CANDIDATE / 'gates.tsv'
with gates_path.open() as stream:
    gates = list(csv.DictReader(stream, delimiter='\t'))
for gate in gates:
    gate['evidence_ref'] = 'evidence/gate-' + gate['gate_id'] + '.json'
with gates_path.open('w', newline='') as stream:
    writer = csv.DictWriter(stream, fieldnames=['gate_id', 'status', 'evidence_ref'], delimiter='\t', lineterminator='\n')
    writer.writeheader()
    writer.writerows(gates)

# Complete all evidence links from real PR88/PR89 producers and PR90 input tables.
refs = set()
for path in CANDIDATE.glob('*.tsv'):
    for row in csv.DictReader(path.open(), delimiter='\t'):
        refs.update(v for k, v in row.items() if k.endswith('_ref') and v not in ('', '-', 'none'))
refs.update(row['evidence_ref'] for row in read(CANDIDATE / 'measurement.json')['records'])
refs.update('evidence/' + name for name in ('acceptance.json', 'storage.json', 'access.json', 'restore.json', 'surveillance.json', 'resolution.json'))
for ref in refs:
    path = CANDIDATE / ref.split('#', 1)[0]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('{"fixture":true,"private_note":"CONFIDENTIAL_SENTINEL_91"}\n')

for key, public in ((KEY, PUBLIC), (OTHER, OTHER_PUBLIC)):
    run('openssl', 'genpkey', '-algorithm', 'ED25519', '-out', key)
    run('openssl', 'pkey', '-in', key, '-pubout', '-out', public)

POLICY = {
    'schema': 'shorthand.c3eco.audit_policy.v1', 'bundle_id': 'audit-2026-001',
    'custodian': 'internal-custodian', 'created_on': AS_OF, 'valid_until': '2027-08-27',
    'retain_until': '2029-08-27', 'legal_hold': False, 'recertification_accepted': True,
    'acceptance_ref': 'evidence/acceptance.json', 'storage_ref': 'evidence/storage.json',
    'access_review_ref': 'evidence/access.json', 'restore_test_ref': 'evidence/restore.json',
    'last_surveillance_on': AS_OF, 'surveillance_ref': 'evidence/surveillance.json',
    'changes': [], 'nonconformities': [], 'public_scope_approved': True,
}
POLICY_PATH = WORK / 'policy.json'
dump(POLICY_PATH, POLICY)
BUNDLE = WORK / 'bundle'
run(TOOL, 'pack', CANDIDATE, POLICY_PATH, KEY, BUNDLE)
receipt = json.loads(run(TOOL, 'verify', BUNDLE, PUBLIC, AS_OF))
schema_check(receipt, 'audit_verification_v1')
schema_check(read(BUNDLE / 'manifest.json'), 'auditor_bundle_v1')
schema_check(POLICY, 'audit_policy_v1')
assert receipt['assessment_replayed'] and receipt['signature_verified'] and receipt['inventory_verified']
assert receipt['lifecycle']['current_review_ready']
assert receipt['lifecycle']['next_surveillance_on'] == '2027-03-09'
assert not receipt['lifecycle']['storage_retention_independently_verified']
assert not any(receipt[k] for k in ('official_certification_granted', 'level_claim_permitted', 'comparative_energy_claim', 'production_claim'))

# A separate implementation verifies the signature and key fingerprint.
payload = WORK / 'signed-payload'
payload.write_bytes(b'shorthand.c3eco.auditor_bundle.v1\n' + (BUNDLE / 'manifest.json').read_bytes())
signature = WORK / 'signature.bin'
signature.write_bytes(bytes.fromhex((BUNDLE / 'manifest.sig').read_text()))
run('openssl', 'pkeyutl', '-verify', '-pubin', '-inkey', PUBLIC, '-rawin', '-in', payload, '-sigfile', signature)
public_der = subprocess.check_output(['openssl', 'pkey', '-pubin', '-in', str(PUBLIC), '-outform', 'DER'])
assert read(BUNDLE / 'manifest.json')['signer_key_id'] == hashlib.sha256(public_der[-32:]).hexdigest()

SECOND = WORK / 'bundle with spaces'
run(TOOL, 'pack', CANDIDATE, POLICY_PATH, KEY, SECOND)
def tree(path):
    return {p.relative_to(path).as_posix(): p.read_bytes() for p in path.rglob('*') if p.is_file()}
assert tree(BUNDLE) == tree(SECOND), 'identical source and key must produce identical bundle bytes'
run(TOOL, 'pack', CANDIDATE, POLICY_PATH, KEY, SECOND, code=2, diagnostic='destination already exists')
assert tree(BUNDLE) == tree(SECOND), 'failed publication modified existing bundle'
run(TOOL, 'verify', BUNDLE, OTHER_PUBLIC, AS_OF, code=2, diagnostic='signature verification failed')
run(TOOL, 'verify', BUNDLE, PUBLIC, '2026-09-08', code=2, diagnostic='precedes bundle creation')
for date, expected in [('2027-03-09', 'surveillance_due'), ('2027-08-28', 'expired')]:
    result = json.loads(run(TOOL, 'verify', BUNDLE, PUBLIC, date, code=3))
    assert result['lifecycle']['status'] == expected
    historic = json.loads(run(TOOL, 'replay', BUNDLE, PUBLIC, date))
    assert historic['assessment_replayed'] and not historic['lifecycle']['current_review_ready']
run(TOOL, 'retention-check', BUNDLE, PUBLIC, '2029-08-27', code=3)
assert json.loads(run(TOOL, 'retention-check', BUNDLE, PUBLIC, '2029-08-28'))['lifecycle']['disposal_policy_eligible']

serial = 0
def clone(source=BUNDLE):
    global serial
    serial += 1
    destination = WORK / ('case-' + str(serial))
    shutil.copytree(source, destination)
    return destination

def reseal(path, mutate=None):
    manifest = read(path / 'manifest.json')
    manifest['artifacts'] = [
        {'path': name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
        for name, data in sorted(tree(path).items()) if name not in ('manifest.json', 'manifest.sig')
    ]
    if mutate:
        mutate(manifest)
    dump(path / 'manifest.json', manifest)
    payload.write_bytes(b'shorthand.c3eco.auditor_bundle.v1\n' + (path / 'manifest.json').read_bytes())
    run('openssl', 'pkeyutl', '-sign', '-inkey', KEY, '-rawin', '-in', payload, '-out', signature)
    (path / 'manifest.sig').write_text(signature.read_bytes().hex())

for kind in ('tamper', 'missing', 'extra'):
    case = clone()
    evidence = case / 'candidate/evidence/storage.json'
    if kind == 'tamper':
        evidence.write_text('altered')
    elif kind == 'missing':
        evidence.unlink()
    else:
        (case / 'injected.txt').write_text('unlisted')
    run(TOOL, 'verify', case, PUBLIC, AS_OF, code=2, diagnostic='inventory or digest mismatch')

case = clone()
(case / 'manifest.json').write_text((case / 'manifest.json').read_text().replace('Ed25519', 'Ed25518'))
run(TOOL, 'verify', case, PUBLIC, AS_OF, code=2, diagnostic='signature verification failed')
case = clone()
assessment = read(case / 'assessment.json')
assessment['scoring']['total_score'] = 1
dump(case / 'assessment.json', assessment)
reseal(case)
run(TOOL, 'verify', case, PUBLIC, AS_OF, code=2, diagnostic='assessment replay differs')
for field, value, diagnostic in [
    ('schema', 'future.v9', 'unsupported bundle'), ('rules_id', 'future.v9', 'unsupported bundle'),
    ('official_certification_granted', True, 'unsupported bundle claim'),
    ('signer_key_id', '0' * 64, 'untrusted signer'), ('extra', 'secret', 'exactly the contract keys')]:
    case = clone()
    reseal(case, lambda m: m.update({field: value}))
    run(TOOL, 'verify', case, PUBLIC, AS_OF, code=2, diagnostic=diagnostic)
case = clone()
(case / 'policy.json').write_text((case / 'policy.json').read_text().replace('"legal_hold":false', '"legal_hold":false,"legal_hold":false'))
reseal(case)
run(TOOL, 'verify', case, PUBLIC, AS_OF, code=2, diagnostic='duplicate object key')
case = clone()
(case / 'rules.json').write_text('{}\n')
reseal(case)
run(TOOL, 'verify', case, PUBLIC, AS_OF, code=2, diagnostic='rule mapping differs')
case = clone()
(case / 'candidate/evidence/storage.json').unlink()
reseal(case)
run(TOOL, 'verify', case, PUBLIC, AS_OF, code=2, diagnostic='unresolved evidence reference')

# Unsafe filesystem representations fail, even before content can be replayed.
case = clone()
(case / 'linked').symlink_to(PUBLIC)
run(TOOL, 'verify', case, PUBLIC, AS_OF, code=2, diagnostic='symlink artifact')
case = clone()
os.link(case / 'candidate/evidence/storage.json', case / 'hardlink')
run(TOOL, 'verify', case, PUBLIC, AS_OF, code=2, diagnostic='hard-linked artifact')
(case / 'hardlink').unlink()
if os.name != 'nt' and sys.platform != 'darwin':
    # Case-sensitive hosts can construct the portable collision negative.
    case = clone()
    (case / 'MANIFEST.json').write_text('collision')
    run(TOOL, 'verify', case, PUBLIC, AS_OF, code=2, diagnostic='case-colliding artifact')
    case = clone()
    (case / 'candidate/Evidence').mkdir()
    (case / 'candidate/Evidence/other.txt').write_text('directory alias')
    run(TOOL, 'verify', case, PUBLIC, AS_OF, code=2, diagnostic='case-colliding artifact')

for updates, diagnostic in [
    ({'retain_until': '2029-08-26'}, '24 months'),
    ({'created_on': '2026-02-30'}, 'invalid date'),
    ({'created_on': '2026-08-31'}, 'measurement is newer than bundle creation'),
    ({'valid_until': '2027-09-09', 'retain_until': '2029-09-09'}, 'exceeds profile validity'),
    ({'recertification_accepted': False}, 'acceptance is required'),
    ({'storage_ref': '../secret'}, 'invalid identifier'),
    ({'storage_ref': 'evidence/CON.txt'}, 'reserved artifact path'),
    ({'extra': True}, 'exactly the contract keys'),
    ({'nonconformities': [{'id': 'nc1', 'severity': 'major', 'status': 'closed', 'due_on': AS_OF, 'resolution_ref': '-'}]}, 'resolution evidence'),
    ({'changes': [{'id': 'c1', 'kind': 'unknown', 'observed_on': AS_OF, 'review_ref': '-'}]}, 'unknown recertification trigger'),
]:
    dump(POLICY_PATH, {**POLICY, **updates})
    destination = WORK / ('invalid-output-' + str(CASES))
    run(TOOL, 'pack', CANDIDATE, POLICY_PATH, KEY, destination, code=2, diagnostic=diagnostic)
    assert not destination.exists(), 'invalid policy published a partial bundle'

def policy_bundle(updates):
    global serial
    serial += 1
    dump(POLICY_PATH, {**POLICY, **updates})
    destination = WORK / ('policy-' + str(serial))
    run(TOOL, 'pack', CANDIDATE, POLICY_PATH, KEY, destination)
    return destination

held = policy_bundle({'legal_hold': True})
run(TOOL, 'retention-check', held, PUBLIC, '2029-08-28', code=3)
changed = policy_bundle({'changes': [{'id': 'model-update', 'kind': 'model', 'observed_on': AS_OF, 'review_ref': '-'}]})
result = json.loads(run(TOOL, 'verify', changed, PUBLIC, AS_OF, code=3))
assert result['lifecycle']['recertification_review_required']
reviewed = policy_bundle({'changes': [{'id': 'model-update', 'kind': 'model', 'observed_on': AS_OF, 'review_ref': 'evidence/resolution.json'}]})
run(TOOL, 'verify', reviewed, PUBLIC, AS_OF)
critical = policy_bundle({'nonconformities': [{'id': 'nc1', 'severity': 'critical', 'status': 'open', 'due_on': '2027-01-01', 'resolution_ref': '-'}]})
run(TOOL, 'verify', critical, PUBLIC, AS_OF, code=3)
run(TOOL, 'retention-check', critical, PUBLIC, '2029-08-28', code=3)
month_end = policy_bundle({'created_on': '2026-10-31', 'last_surveillance_on': '2026-10-31'})
assert json.loads(run(TOOL, 'verify', month_end, PUBLIC, '2026-10-31'))['lifecycle']['next_surveillance_on'] == '2027-04-30'
run(TOOL, 'verify', month_end, PUBLIC, AS_OF, code=2, diagnostic='precedes bundle creation')
closed = policy_bundle({'nonconformities': [{'id': 'nc1', 'severity': 'critical', 'status': 'closed', 'due_on': '2027-01-01', 'resolution_ref': 'evidence/resolution.json'}]})
run(TOOL, 'verify', closed, PUBLIC, AS_OF)
minor = policy_bundle({'nonconformities': [{'id': 'nc1', 'severity': 'minor', 'status': 'open', 'due_on': '2026-10-01', 'resolution_ref': '-'}]})
run(TOOL, 'verify', minor, PUBLIC, AS_OF)
run(TOOL, 'verify', minor, PUBLIC, '2026-10-01', code=3)

PUBLIC_BUNDLE = WORK / 'public-bundle'
run(TOOL, 'export-public', BUNDLE, PUBLIC, AS_OF, KEY, PUBLIC_BUNDLE)
public_receipt = json.loads(run(TOOL, 'verify-public', PUBLIC_BUNDLE, PUBLIC, AS_OF))
schema_check(public_receipt, 'public_verification_v1')
schema_check(read(PUBLIC_BUNDLE / 'manifest.json'), 'auditor_bundle_v1')
schema_check(read(PUBLIC_BUNDLE / 'report.json'), 'public_audit_report_v1')
assert public_receipt['signature_verified'] and not public_receipt['private_assessment_replayed']
public_bytes = b'\n'.join(tree(PUBLIC_BUNDLE).values())
for forbidden in (b'CONFIDENTIAL_SENTINEL_91', b'internal-custodian', b'PRIVATE KEY', b'evidence/storage', b'recommended_level', b'diamond'):
    assert forbidden not in public_bytes, forbidden
assert len(tree(PUBLIC_BUNDLE)) == 4
assert read(PUBLIC_BUNDLE / 'report.json')['source_manifest_sha256'] == receipt['manifest_sha256']
run(TOOL, 'verify-public', PUBLIC_BUNDLE, PUBLIC, '2027-03-09', code=2, diagnostic='surveillance is due')
run(TOOL, 'export-public', BUNDLE, PUBLIC, '2027-03-09', KEY, WORK / 'expired-export', code=2, diagnostic='review is blocked')
no_consent = policy_bundle({'public_scope_approved': False})
run(TOOL, 'export-public', no_consent, PUBLIC, AS_OF, KEY, WORK / 'no-consent-export', code=2, diagnostic='explicit approval')
run(TOOL, 'export-public', BUNDLE, PUBLIC, AS_OF, OTHER, WORK / 'wrong-export', code=2, diagnostic='trusted bundle signer')
case = clone(PUBLIC_BUNDLE)
(case / 'report.md').write_text('untrusted report')
run(TOOL, 'verify-public', case, PUBLIC, AS_OF, code=2, diagnostic='digest mismatch')
case = clone(PUBLIC_BUNDLE)
report = read(case / 'report.json'); report['product']['secret'] = 'leak'; dump(case / 'report.json', report)
reseal(case)
run(TOOL, 'verify-public', case, PUBLIC, AS_OF, code=2, diagnostic='exactly the contract keys')
case = clone(PUBLIC_BUNDLE)
(case / 'report.md').write_text('CONFIDENTIAL_SENTINEL_91')
reseal(case)
run(TOOL, 'verify-public', case, PUBLIC, AS_OF, code=2, diagnostic='redaction contract')
case = clone(PUBLIC_BUNDLE)
report = read(case / 'report.json'); report['private_evidence'] = 'leak'; dump(case / 'report.json', report)
reseal(case)
run(TOOL, 'verify-public', case, PUBLIC, AS_OF, code=2, diagnostic='exactly the contract keys')

INTAKE = WORK / 'readiness.json'
intake = {'schema': 'shorthand.c3eco.readiness_intake.v1', 'product_id': 'example', 'functional_unit': 'request', 'boundary': 'service', 'estimates': []}
dump(INTAKE, intake)
assert json.loads(run(TOOL, 'readiness', INTAKE))['readiness_state'] == 'Registered'
intake['estimates'] = [{'component': 'compute', 'energy_kwh': 10, 'carbon_factor_gco2e_per_kwh': 400,
                      'uncertainty_percent': 25, 'method': 'documented provider estimate', 'source_ref': 'provider/report'}]
dump(INTAKE, intake)
result = json.loads(run(TOOL, 'readiness', INTAKE))
schema_check(intake, 'readiness_intake_v1')
schema_check(result, 'readiness_result_v1')
assert result['readiness_state'] == 'Candidate' and result['measurement_quality'] == 'MQ1'
assert not result['measured_workbook_produced'] and not result['official_certification_granted']
intake['estimates'][0]['source_ref'] = ''
dump(INTAKE, intake)
run(TOOL, 'readiness', INTAKE, code=2, diagnostic='provenance required')
assert not list(WORK.glob('.shorthand-audit-*')), 'staging directories leaked after failure'
print(f'PASS auditor bundle integration cases={CASES}; independent signatures, replay, lifecycle and public redaction')
