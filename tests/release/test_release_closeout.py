"""Exercise real closeout code with source evidence and explicit synthetic API/RC records."""
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT, TOOL = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
spec = importlib.util.spec_from_file_location('release_closeout', TOOL)
closeout = importlib.util.module_from_spec(spec)
spec.loader.exec_module(closeout)
REVISION = os.environ.get('GITHUB_SHA')
if not REVISION:
    REVISION = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
assert re.fullmatch('[0-9a-f]{40}', REVISION)
CASES = 0


def dump(path, value):
    path.write_bytes((json.dumps(value, sort_keys=True, separators=(',', ':')) + '\n').encode('utf-8'))


def read(path):
    return json.loads(path.read_bytes())


def schema_check(value, schema):
    if 'anyOf' in schema:
        for child in schema['anyOf']:
            try:
                schema_check(value, child)
                return
            except AssertionError:
                pass
        raise AssertionError('no schema alternative matched')
    if 'const' in schema:
        assert type(value) is type(schema['const']) and value == schema['const']
    if 'enum' in schema:
        assert value in schema['enum']
    kind = schema.get('type')
    if kind == 'object':
        assert type(value) is dict and set(schema.get('required', [])) <= value.keys()
        if schema.get('additionalProperties') is False:
            assert value.keys() <= schema['properties'].keys()
        for key, child in value.items():
            schema_check(child, schema['properties'][key])
    elif kind == 'array':
        assert type(value) is list
        assert schema.get('minItems', 0) <= len(value) <= schema.get('maxItems', float('inf'))
        if schema.get('uniqueItems'):
            assert len({json.dumps(v, sort_keys=True) for v in value}) == len(value)
        for child in value:
            schema_check(child, schema['items'])
    elif kind == 'string':
        assert type(value) is str and len(value) >= schema.get('minLength', 0)
        if 'pattern' in schema:
            assert re.search(schema['pattern'], value)
    elif kind == 'integer':
        assert type(value) is int and schema.get('minimum', -float('inf')) <= value <= schema.get('maximum', float('inf'))
    elif kind == 'boolean':
        assert type(value) is bool
    elif kind == 'null':
        assert value is None


def invalid(action, message):
    global CASES
    CASES += 1
    try:
        action()
    except (ValueError, OSError, KeyError, TypeError) as error:
        assert message in str(error), (message, str(error))
    else:
        raise AssertionError('invalid closeout evidence accepted: ' + message)


def blocked(report, reason):
    global CASES
    CASES += 1
    schema_check(report, read(ROOT / 'schemas/release/release_closeout_v1.schema.json'))
    assert report['decision'] == 'blocked_by_retained_evidence' and reason in report['blockers']
    for key in ('publication_authorized', 'production_claim', 'official_certification_granted',
                'level_claim_permitted', 'comparative_energy_claim', 'accelerator_production_support'):
        assert report[key] is False


base = closeout.evaluate(ROOT, REVISION)
blocked(base, 'live_github_release_state_unverified')
blocked(base, 'executed_rc_report_missing')
assert base == closeout.evaluate(ROOT, REVISION), 'report is not deterministic'
assert len(base['audit_findings']) == 25
assert len([r for r in base['audit_findings'] if r['status'] == 'evidence_pending']) == 13
assert base['compiler_blockers'] == ['TST017', 'TST025', 'TST026']
assert base['certification_blockers'] == ['B', 'C', 'D', 'G8', 'S12', 'S9']
schema_check(read(ROOT / closeout.LEDGER), read(ROOT / 'schemas/release/release_closeout_ledger_v1.schema.json'))
for item in base['inputs']:
    data = (ROOT / item['path']).read_bytes()
    assert item['sha256'] == hashlib.sha256(data).hexdigest() and item['bytes'] == len(data)

with tempfile.TemporaryDirectory(prefix='shorthand-pr102-') as temp:
    work = Path(temp)
    pristine = work / 'source'
    for item in base['inputs']:
        target = pristine / item['path']
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / item['path'], target)

    def clone():
        path = work / ('case-' + str(CASES))
        shutil.copytree(pristine, path)
        return path

    for mutation, diagnostic in [('missing', 'all 25'), ('duplicate', 'duplicate or unknown'),
                                  ('severity', 'was rewritten'), ('title', 'was rewritten'),
                                  ('promotion', 'unsupported audit evidence promotion'),
                                  ('scope', 'unsupported audit scope promotion'),
                                  ('digest', 'audit digest mismatch'), ('extra', 'unexpected or missing')]:
        root = clone()
        ledger = read(root / closeout.LEDGER)
        if mutation == 'missing': ledger['gaps'].pop()
        elif mutation == 'duplicate': ledger['gaps'][1] = ledger['gaps'][0]
        elif mutation == 'severity': ledger['gaps'][0]['severity'] = 'S2'
        elif mutation == 'title': ledger['gaps'][0]['title'] = 'rewritten acceptance'
        elif mutation == 'promotion': ledger['gaps'][1]['status'] = 'implemented'
        elif mutation == 'scope': ledger['gaps'][8]['status'] = 'implemented'
        elif mutation == 'digest': ledger['audit_sha256'] = '0' * 64
        else: ledger['production_claim'] = True
        dump(root / closeout.LEDGER, ledger)
        invalid(lambda: closeout.evaluate(root, REVISION), diagnostic)
    for name in closeout.PUBLIC_DOCS:
        root = clone()
        path = root / name
        path.write_bytes(path.read_bytes().replace(b'controlled_beta', b'enterprise_ga'))
        invalid(lambda: closeout.evaluate(root, REVISION), 'public capability block drift')
    for field in ('production_claim', 'official_certification_granted', 'accelerator_production_support'):
        root = clone()
        path = root / closeout.TRUTH
        path.write_bytes(path.read_bytes().replace((field + '\tfalse').encode(), (field + '\ttrue').encode()))
        invalid(lambda: closeout.evaluate(root, REVISION), 'unsupported public capability')
    root = clone()
    for name in closeout.PUBLIC_DOCS:
        path = root / name
        path.write_bytes(path.read_bytes().replace(b'\r\n', b'\n').replace(b'\n', b'\r\n'))
    blocked(closeout.evaluate(root, REVISION), 'executed_rc_report_missing')
    root = clone()
    path = root / closeout.TRUTH
    path.write_bytes(path.read_bytes().replace(b'ga_publication_authorized\tfalse', b'ga_publication_authorized\ttrue'))
    invalid(lambda: closeout.evaluate(root, REVISION), 'unsupported release publication policy')
    root = clone()
    path = root / closeout.MATRIX
    rows = path.read_text().splitlines()
    for index, line in enumerate(rows):
        parts = line.split('\t')
        if parts[0] == 'TST017':
            parts[2] = 'implemented'
            rows[index] = '\t'.join(parts)
    path.write_bytes(('\n'.join(rows) + '\n').encode())
    invalid(lambda: closeout.evaluate(root, REVISION), 'unsupported compiler evidence promotion')
    root = clone()
    path = root / closeout.TRACE
    rows = path.read_text().splitlines()
    rows = ['\t'.join(line.split('\t')[:4] + ['implemented'] + line.split('\t')[5:]) if line.startswith('G8\t') else line for line in rows]
    path.write_bytes(('\n'.join(rows) + '\n').encode())
    invalid(lambda: closeout.evaluate(root, REVISION), 'unsupported certification evidence promotion')
    for value in (b'{"a":1,"a":2}', b'{"a":NaN}', b'{"a":1e9999}'):
        invalid(lambda: closeout.decode(value), 'JSON')
    invalid(lambda: closeout.snapshot(pristine, '../outside'), 'unsafe evidence path')
    invalid(lambda: closeout.snapshot(pristine, 'C:/outside'), 'unsafe evidence path')
    root = clone()
    target = root / 'README.md'
    target.unlink()
    os.link(pristine / 'README.md', target)
    invalid(lambda: closeout.evaluate(root, REVISION), 'invalid or oversized evidence file')
    target.unlink()
    if os.name != 'nt':  # POSIX symlink boundary; Windows also rejects reparse attributes.
        root = clone()
        target = root / 'README.md'
        target.unlink()
        target.symlink_to(pristine / 'README.md')
        invalid(lambda: closeout.evaluate(root, REVISION), 'symlink evidence is forbidden')

    rc = {'schema': 'shorthand.enterprise.pilot_rc.v1', 'contract_version': 'shorthand.enterprise.pilot_rc.v1',
          'commit': REVISION, 'execution_mode': 'executed', 'production_scope': 'linux-x64-cpu-v1',
          'production_backend': 'onnxruntime_cpu', 'production_device': 'cpu', 'production_claim': False,
          'accelerator_production_support': False, 'mandatory_skips': 0,
          'release_candidate_decision': 'blocked_by_retained_evidence',
          'lifecycle': dict.fromkeys(('clean_install', 'same_version_upgrade', 'rollback', 'uninstall'), True),
          'deployment': dict.fromkeys(('contract_checked', 'serving_soak_checked', 'disaster_recovery_checked'), True),
          'compiler_blockers': [{'id': key, 'area': 'synthetic fixture', 'status': 'partial', 'closure_pr': 'PR102'} for key in base['compiler_blockers']],
          'traceability_blockers': [{'id': key, 'requirement': 'synthetic fixture', 'status': 'partial', 'closure_pr': 'PR102'} for key in base['certification_blockers']]}
    rc_path = work / 'rc.json'
    dump(rc_path, rc)
    blocked(closeout.evaluate(ROOT, REVISION, rc_path), 'rc_execution_requires_independent_review')
    for field, value, message in [('commit', '0' * 40, 'RC identity mismatch'),
                                  ('production_claim', True, 'claim or skip violation'),
                                  ('mandatory_skips', 1, 'claim or skip violation'),
                                  ('mandatory_skips', False, 'claim or skip violation'),
                                  ('release_candidate_decision', 'eligible_for_review', 'contradicts retained blockers'),
                                  ('compiler_blockers', [], 'blocker inventory differs')]:
        dump(rc_path, dict(rc, **{field: value}))
        invalid(lambda: closeout.evaluate(ROOT, REVISION, rc_path), message)
    changed = copy.deepcopy(rc)
    changed['lifecycle']['clean_install'] = 'true'
    dump(rc_path, changed)
    invalid(lambda: closeout.evaluate(ROOT, REVISION, rc_path), 'flags must be boolean')
    changed['lifecycle']['clean_install'] = False
    dump(rc_path, changed)
    blocked(closeout.evaluate(ROOT, REVISION, rc_path), 'rc_execution_incomplete')

    head, tree, master = 'b' * 40, 'c' * 40, 'd' * 40
    repository = 'amarbanerjee23/ShortHand_Compiler'
    api = {'commits/' + REVISION: {'sha': REVISION, 'commit': {'tree': {'sha': tree}}},
           'pulls/102': {'number': 102, 'base': {'ref': 'master', 'repo': {'full_name': repository}},
                         'head': {'sha': head}, 'merged': True, 'merge_commit_sha': REVISION},
           'commits/' + head: {'sha': head, 'commit': {'tree': {'sha': tree}}},
           'commits/' + head + '/status?per_page=100': {'sha': head, 'statuses': [{'context': name, 'state': 'success'} for name in closeout.CONTEXTS]},
           'branches/master': {'name': 'master', 'commit': {'sha': master}, 'protected': True},
           'compare/' + REVISION + '...' + master: {'status': 'ahead', 'merge_base_commit': {'sha': REVISION}}}
    get = lambda repo, endpoint: copy.deepcopy(api[endpoint])
    observation = closeout.observe_github(repository, REVISION, 102, get)
    assert closeout.assess_github(observation, REVISION) == []
    blocked(closeout.evaluate(ROOT, REVISION, observation=observation), 'github_observation_not_live')
    for field, value, reason in [('merged', False, 'source_is_not_the_recorded_merged_pr'),
                                ('source_on_master', False, 'source_not_on_current_master_lineage'),
                                ('reviewed_tree', 'e' * 40, 'release_tree_differs_from_reviewed_head'),
                                ('master_protected', False, 'master_protection_not_enforced'),
                                ('required_contexts', {}, 'required_ci_not_successful:ci / ubuntu (push)')]:
        blocked(closeout.evaluate(ROOT, REVISION, observation=dict(observation, **{field: value})), reason)
    status = api['commits/' + head + '/status?per_page=100']
    status['statuses'][0]['state'] = 'failure'
    failed = closeout.observe_github(repository, REVISION, 102, get)
    blocked(closeout.evaluate(ROOT, REVISION, observation=failed), 'required_ci_not_successful:ci / ubuntu (push)')
    status['statuses'].append(status['statuses'][0])
    invalid(lambda: closeout.observe_github(repository, REVISION, 102, get), 'ambiguous required')
    status['statuses'].pop()
    status['sha'] = 'f' * 40
    invalid(lambda: closeout.observe_github(repository, REVISION, 102, get), 'CI commit mismatch')
    invalid(lambda: closeout.NoRedirect().redirect_request(None, None, 302, '', {}, 'https://untrusted.invalid'), 'redirects')

    output = work / 'report.json'
    # Execute the actual workflow shell with a recording command, not a policy reimplementation.
    workflow = (ROOT / '.github/workflows/release.yml').read_text(encoding='utf-8')
    gate = workflow.split('      - name: Validate release claims and block unauthorized GA publication\n', 1)[1]
    gate = gate.split('      - uses:', 1)[0].split('        run: |\n', 1)[1]
    shell = '\n'.join(line[10:] for line in gate.splitlines())
    recorder = 'python3() { printf "%s\\n" "$@"; }\n'
    for version, publish, require_ga in [('v1.0.0', 'true', True), ('v1.0.0-rc.1', 'true', False),
                                         ('v1.0.0', 'false', False), ('v1.0.0-rc.2', 'false', False)]:
        env = dict(os.environ, REQUESTED_VERSION=version, PUBLISH=publish, GITHUB_SHA=REVISION)
        arguments = subprocess.check_output(['bash', '-c', recorder + shell], env=env, text=True).splitlines()
        assert arguments[0] == 'scripts/release_closeout.py'
        assert ('--require-ga' in arguments) is require_ga and ('--github' in arguments) is require_ga
        assert arguments[arguments.index('--revision') + 1] == REVISION
        CASES += 1
    command = [sys.executable, str(TOOL), '--root', str(ROOT), '--revision', REVISION, '--report', str(output)]
    run = subprocess.run(command + ['--require-ga'], capture_output=True, text=True, timeout=30)
    assert run.returncode == 3, (run.stdout, run.stderr)
    blocked(read(output), 'protected_signed_release_exercise_pending')
    before = (pristine / closeout.TRUTH).read_bytes()
    run = subprocess.run([sys.executable, str(TOOL), '--root', str(pristine), '--revision', REVISION,
                          '--report', str(pristine / closeout.TRUTH)], capture_output=True, text=True, timeout=30)
    assert run.returncode == 2 and 'overwrite source evidence' in run.stderr
    assert (pristine / closeout.TRUTH).read_bytes() == before

print(f'PASS PR102 release closeout cases={CASES}; audit inventory, claims, source identity, RC and publication safety')
