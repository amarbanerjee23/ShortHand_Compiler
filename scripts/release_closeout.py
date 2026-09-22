#!/usr/bin/env python3
"""PR102: bounded release review, never certification or publication authority."""
import argparse
import csv
import hashlib
import io
import json
import math
import os
from pathlib import Path, PurePosixPath
import re
import stat
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request

SCHEMA = 'shorthand.release.closeout.v1'
LEDGER = 'docs/release_closeout_ledger.json'
AUDIT = 'docs/ENTERPRISE_AI_C3ECO_GAP_ASSESSMENT.md'
TRUTH = 'docs/production_truth.tsv'
MATRIX = 'tests/coverage/compiler_test_coverage_matrix.tsv'
TRACE = 'docs/c3eco_traceability.tsv'
PUBLIC_DOCS = ('README.md', 'docs/backend_compatibility_matrix.md', 'docs/public_release_readiness.md')
BEGIN = '<!-- BEGIN SHORTHAND VERIFIED CAPABILITIES -->'
END = '<!-- END SHORTHAND VERIFIED CAPABILITIES -->'
CONTEXTS = ('ci / ubuntu (push)', 'ci / ubuntu (pull_request)')
MAX_BYTES = 4 * 1024 * 1024
STATES = ('implemented', 'scoped', 'evidence_pending')
PENDING_AUDIT = {'SH-EA-%03d' % i for i in (2, 3, 5, 6, 7, 11, 12, 13, 15, 18, 20, 24, 25)}
SHA = re.compile(r'[0-9a-f]{40}\Z')
DIGEST = re.compile(r'[0-9a-f]{64}\Z')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def exact(value, keys, label):
    require(type(value) is dict and set(value) == set(keys.split()), label + ': unexpected or missing fields')


def pairs(items):
    result = {}
    for key, value in items:
        require(key not in result, 'duplicate JSON key: ' + key)
        result[key] = value
    return result


def decode(data):
    def invalid(value):
        raise ValueError('non-finite JSON constant: ' + value)
    def finite(value):
        number = float(value)
        require(math.isfinite(number), 'non-finite JSON number')
        return number
    return json.loads(data, object_pairs_hook=pairs, parse_constant=invalid, parse_float=finite)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def snapshot(root, name):
    require(type(name) is str and 0 < len(name) <= 512 and '\\' not in name,
            'unsafe evidence path')
    parts = PurePosixPath(name).parts
    require(parts and not name.startswith('/') and all(p not in ('.', '..') for p in parts)
            and str(PurePosixPath(name)) == name and ':' not in name, 'unsafe evidence path')
    path = root
    for part in parts:
        path = path / part
        require(not path.is_symlink(), 'symlink evidence is forbidden: ' + name)
        require(not (getattr(path.lstat(), 'st_file_attributes', 0) & 0x400),
                'reparse-point evidence is forbidden: ' + name)
    with path.open('rb') as stream:
        info = os.fstat(stream.fileno())
        require(stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and 0 < info.st_size <= MAX_BYTES,
                'invalid or oversized evidence file: ' + name)
        data = stream.read(MAX_BYTES + 1)
    require(len(data) == info.st_size, 'evidence changed while reading: ' + name)
    return data


def table(data, header):
    rows = list(csv.reader(io.StringIO(data.decode('utf-8')), delimiter='\t'))
    fields = header.split()
    require(rows and rows[0] == fields, 'invalid evidence table header')
    require(len(rows) <= 1000 and all(len(r) == len(fields) for r in rows[1:]), 'invalid evidence table rows')
    require(len({r[0] for r in rows[1:]}) == len(rows) - 1, 'duplicate evidence table ID')
    return [dict(zip(fields, row)) for row in rows[1:]]


def capabilities(truth):
    expected = {'current_maturity': 'controlled_beta', 'active_language_version': 'beta-0.7',
                'production_backend_scope': 'linux-x64-cpu-v1', 'production_claim': 'false',
                'accelerator_production_support': 'false', 'official_certification_granted': 'false',
                'level_claim_permitted': 'false', 'comparative_energy_claim': 'false',
                'mandatory_test_skip_policy': 'forbidden'}
    require(all(truth.get(k) == v for k, v in expected.items()), 'unsupported public capability or claim')
    return {k: (v == 'true' if v in ('false', 'true') else v) for k, v in expected.items()}


def render_public(values):
    return '\n'.join((BEGIN,
        'Current release scope: **' + values['current_maturity'] + '**; language **' + values['active_language_version'] + '**.',
        'Qualified AI execution scope: **' + values['production_backend_scope'] + '** (ONNX Runtime CPU).',
        'Compiler/runtime execution uses C++/LLVM. Qualification and release tooling also require Python 3.',
        'Packages use the curated offline registry and deterministic lockfiles; nested ownership and public service ingress remain outside the declared scope.',
        'C3-ECO support produces candidate evidence against draft v0.6 plus the dated v0.7 inclusion overlay. It does not grant certification or a certification level.',
        'Production readiness, accelerator production support, comparative energy superiority and universal lowest-carbon claims are not authorized.',
        END))


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, response, code, message, headers, url):
        raise ValueError('GitHub evidence redirects are not allowed')


def github_get(repository, endpoint):
    require(re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository), 'invalid repository')
    require(re.fullmatch(r'[A-Za-z0-9_./?=&-]+', endpoint), 'invalid GitHub endpoint')
    headers = {'Accept': 'application/vnd.github+json', 'X-GitHub-Api-Version': '2022-11-28',
               'User-Agent': 'ShortHand-release-closeout'}
    token = os.environ.get('GH_TOKEN') or os.environ.get('GITHUB_TOKEN')
    if token:
        headers['Authorization'] = 'Bearer ' + token
    request = urllib.request.Request('https://api.github.com/repos/' + repository + '/' + endpoint, headers=headers)
    with urllib.request.build_opener(NoRedirect).open(request, timeout=20) as response:
        data = response.read(MAX_BYTES + 1)
    require(len(data) <= MAX_BYTES, 'oversized GitHub response')
    return decode(data)


def observe_github(repository, revision, current_pr, get=github_get):
    """Read live facts; fixture callers cannot set the report's live flag."""
    source = get(repository, 'commits/' + revision)
    pr = get(repository, 'pulls/' + str(current_pr))
    require(source['sha'] == revision and SHA.fullmatch(source['commit']['tree']['sha']), 'GitHub source identity mismatch')
    require(pr['number'] == current_pr and pr['base']['ref'] == 'master'
            and pr['base']['repo']['full_name'] == repository, 'GitHub PR identity mismatch')
    head = pr['head']['sha']
    require(type(head) is str and SHA.fullmatch(head), 'invalid reviewed head')
    reviewed = get(repository, 'commits/' + head)
    status = get(repository, 'commits/' + head + '/status?per_page=100')
    master = get(repository, 'branches/master')
    require(reviewed['sha'] == head and status['sha'] == head, 'GitHub CI commit mismatch')
    require(SHA.fullmatch(reviewed['commit']['tree']['sha']), 'invalid reviewed tree')
    require(pr['merge_commit_sha'] is None or SHA.fullmatch(pr['merge_commit_sha']), 'invalid merge commit')
    require(type(pr['merged']) is bool and type(master['protected']) is bool, 'invalid GitHub policy state')
    require(master['name'] == 'master' and SHA.fullmatch(master['commit']['sha']), 'invalid GitHub master identity')
    lineage = get(repository, 'compare/' + revision + '...' + master['commit']['sha'])
    on_master = lineage['status'] in ('ahead', 'identical') and lineage['merge_base_commit']['sha'] == revision
    require(type(status['statuses']) is list and len(status['statuses']) <= 100, 'invalid GitHub statuses')
    contexts = {}
    for item in status['statuses']:
        name = item['context']
        if name in CONTEXTS:
            require(name not in contexts, 'ambiguous required GitHub status')
            require(item['state'] in ('success', 'pending', 'failure', 'error'), 'invalid GitHub status')
            contexts[name] = item['state']
    return {'source_commit': revision, 'reviewed_head': head,
            'source_tree': source['commit']['tree']['sha'], 'reviewed_tree': reviewed['commit']['tree']['sha'],
            'master_commit': master['commit']['sha'], 'master_protected': master['protected'],
            'source_on_master': on_master,
            'pull_number': current_pr, 'merged': pr['merged'], 'merge_commit': pr['merge_commit_sha'],
            'required_contexts': contexts}


def assess_github(observation, revision):
    reasons = []
    if not observation:
        return ['live_github_release_state_unverified']
    require(observation['source_commit'] == revision, 'observed source revision mismatch')
    if not observation['merged'] or observation['merge_commit'] != revision:
        reasons.append('source_is_not_the_recorded_merged_pr')
    if not observation['source_on_master']:
        reasons.append('source_not_on_current_master_lineage')
    if observation['source_tree'] != observation['reviewed_tree']:
        reasons.append('release_tree_differs_from_reviewed_head')
    if not observation['master_protected']:
        reasons.append('master_protection_not_enforced')
    for context in CONTEXTS:
        if observation['required_contexts'].get(context) != 'success':
            reasons.append('required_ci_not_successful:' + context)
    return reasons


def evaluate(root, revision, rc_path=None, observation=None, live=False):
    require(type(revision) is str and SHA.fullmatch(revision), 'exact source commit is required')
    root = root.resolve(strict=True)
    inputs = {}
    def take(name):
        if name not in inputs:
            inputs[name] = snapshot(root, name)
        return inputs[name]
    truth = {r['key']: r['value'] for r in table(take(TRUTH), 'key value')}
    public = capabilities(truth)
    require(truth.get('ga_publication_authorized') == 'false' and truth.get('release_closeout_contract') == SCHEMA,
            'unsupported release publication policy')
    require(truth.get('current_github_pr') == '102' and truth.get('last_merged_github_pr') == '101'
            and truth.get('remaining_implementation_prs_after_current') == '0', 'closeout roadmap identity drift')
    matrix = table(take(MATRIX), 'id area status existing_evidence missing_evidence closure_pr production_blocker')
    trace = table(take(TRACE), 'id category requirement source status implementation_evidence verification_evidence owner closure_target production_blocker')
    require({r['id'] for r in matrix} == {'TST%03d' % i for i in range(1, 40)}, 'incomplete compiler coverage inventory')
    require({r['id'] for r in trace} == ({'G' + str(i) for i in range(1, 15)} | set('ABCDEFGHIJK') | {'S9', 'S12'}),
            'incomplete certification traceability inventory')
    for row in matrix + trace:
        require(row['status'] in ('implemented', 'partial', 'open') and row['production_blocker'] in ('yes', 'no'),
                'invalid blocker classification')
        require(row['status'] == 'implemented' or row['production_blocker'] == 'yes', 'unresolved evidence cannot lose its blocker')
    compiler = sorted(r['id'] for r in matrix if r['status'] != 'implemented')
    certification = sorted(r['id'] for r in trace if r['status'] != 'implemented')
    require(compiler == ['TST017', 'TST025', 'TST026'], 'unsupported compiler evidence promotion')
    require(certification == ['B', 'C', 'D', 'G8', 'S12', 'S9'], 'unsupported certification evidence promotion')
    ledger = decode(take(LEDGER))
    exact(ledger, 'schema audit_sha256 production_scope gaps', 'audit ledger')
    require(ledger['schema'] == 'shorthand.release.closeout_ledger.v1' and ledger['production_scope'] == public['production_backend_scope'],
            'unsupported closeout ledger')
    require(ledger['audit_sha256'] == sha(take(AUDIT)), 'historical audit digest mismatch')
    historical = re.findall(r'^### (SH-EA-\d{3}) \| ([^|]+) \| (.+)$', take(AUDIT).decode('utf-8'), re.M)
    expected = {name: (severity.strip(), title.strip()) for name, severity, title in historical}
    require(len(expected) == 25 and set(expected) == {'SH-EA-%03d' % i for i in range(1, 26)}, 'historical audit inventory changed')
    require(type(ledger['gaps']) is list and len(ledger['gaps']) == 25, 'all 25 audit findings must be reconciled')
    gaps, seen = [], set()
    for row in ledger['gaps']:
        exact(row, 'id severity title status disposition evidence remaining_exit owner', 'audit finding')
        name = row['id']
        require(name in expected and name not in seen, 'missing, duplicate or unknown audit finding')
        seen.add(name)
        require((row['severity'], row['title']) == expected[name], 'historical audit finding was rewritten')
        require(row['status'] in STATES, 'unsupported audit finding state')
        for field in ('disposition', 'remaining_exit', 'owner'):
            require(type(row[field]) is str and 0 < len(row[field]) <= 2000, 'missing audit disposition or exit')
        require(type(row['evidence']) is list and 1 <= len(row['evidence']) <= 12
                and len(set(row['evidence'])) == len(row['evidence']), 'invalid audit evidence inventory')
        for path in row['evidence']:
            take(path)
        gaps.append(row)
    require({r['id'] for r in gaps if r['status'] == 'evidence_pending'} == PENDING_AUDIT,
            'unsupported audit evidence promotion or omission')
    require({r['id'] for r in gaps if r['status'] == 'scoped'} ==
            {'SH-EA-%03d' % i for i in (9, 10, 14, 16, 17, 21)},
            'unsupported audit scope promotion or omission')
    for name in PUBLIC_DOCS:
        text = take(name).decode('utf-8').replace('\r\n', '\n')
        require(text.count(BEGIN) == text.count(END) == 1 and render_public(public) in text, 'public capability block drift: ' + name)
    reasons = ['compiler:' + name for name in compiler] + ['certification:' + name for name in certification]
    reasons += ['audit:' + row['id'] for row in gaps if row['status'] == 'evidence_pending']
    reasons += assess_github(observation, revision)
    if observation and not live:
        reasons.append('github_observation_not_live')
    # V1 has no independently validated protected-exercise receipt.
    reasons.append('protected_signed_release_exercise_pending')
    rc = None
    if rc_path is None:
        reasons.append('executed_rc_report_missing')
    else:
        require(not rc_path.is_symlink(), 'RC evidence cannot be a symlink')
        raw = snapshot(rc_path.absolute().parent, rc_path.name)
        rc = decode(raw)
        exact(rc, 'schema contract_version commit execution_mode production_scope production_backend production_device production_claim accelerator_production_support mandatory_skips release_candidate_decision lifecycle deployment compiler_blockers traceability_blockers', 'RC report')
        require(rc['schema'] == rc['contract_version'] == 'shorthand.enterprise.pilot_rc.v1'
                and rc['commit'] == revision and rc['production_scope'] == public['production_backend_scope']
                and rc['production_backend'] == 'onnxruntime_cpu' and rc['production_device'] == 'cpu', 'RC identity mismatch')
        require(rc['production_claim'] is False and rc['accelerator_production_support'] is False
                and type(rc['mandatory_skips']) is int and rc['mandatory_skips'] == 0, 'RC claim or skip violation')
        exact(rc['lifecycle'], 'clean_install same_version_upgrade rollback uninstall', 'RC lifecycle')
        exact(rc['deployment'], 'contract_checked serving_soak_checked disaster_recovery_checked', 'RC deployment')
        flags = list(rc['lifecycle'].values()) + list(rc['deployment'].values())
        require(all(type(v) is bool for v in flags), 'RC execution flags must be boolean')
        require(rc['execution_mode'] in ('contract', 'executed'), 'invalid RC execution mode')
        require(rc['release_candidate_decision'] == 'blocked_by_retained_evidence', 'RC eligibility contradicts retained blockers')
        for rows, keys, expected_ids in ((rc['compiler_blockers'], 'id area status closure_pr', compiler),
                                        (rc['traceability_blockers'], 'id requirement status closure_pr', certification)):
            require(type(rows) is list, 'invalid RC blocker inventory')
            for row in rows:
                exact(row, keys, 'RC blocker')
                require(row['status'] in ('partial', 'open'), 'invalid RC blocker status')
                require(all(type(v) is str and 0 < len(v) <= 2000 for v in row.values()), 'invalid RC blocker fields')
                require(re.fullmatch(r'PR[0-9]+', row['closure_pr']), 'invalid RC closure target')
            require(sorted(r['id'] for r in rows) == expected_ids, 'RC blocker inventory differs from source')
        if rc['execution_mode'] != 'executed' or not all(flags):
            reasons.append('rc_execution_incomplete')
        # A JSON receipt alone does not authenticate execution or a physical DR event.
        reasons.append('rc_execution_requires_independent_review')
        rc = {'sha256': sha(raw), 'execution_mode': rc['execution_mode']}
    reasons = sorted(set(reasons))
    return {'schema': SCHEMA, 'source_commit': revision, 'production_scope': public['production_backend_scope'],
            'maturity': public['current_maturity'], 'decision': 'blocked_by_retained_evidence' if reasons else 'eligible_for_manual_review',
            'publication_authorized': False, 'production_claim': False, 'official_certification_granted': False,
            'level_claim_permitted': False, 'comparative_energy_claim': False, 'accelerator_production_support': False,
            'planned_implementation_prs_after_current': 0, 'capabilities': public,
            'audit_findings': sorted(gaps, key=lambda r: r['id']), 'compiler_blockers': compiler,
            'certification_blockers': certification, 'blockers': reasons, 'github': observation,
            'live_github_observation': live, 'rc_report': rc,
            'inputs': [{'path': name, 'sha256': sha(data), 'bytes': len(data)} for name, data in sorted(inputs.items())]}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', required=True, type=Path)
    parser.add_argument('--revision', required=True)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--rc-report', type=Path)
    parser.add_argument('--github', action='store_true')
    parser.add_argument('--repository', default='amarbanerjee23/ShortHand_Compiler')
    parser.add_argument('--require-ga', action='store_true', help='nonzero unless GA publication is authorized')
    args = parser.parse_args(argv)
    try:
        require(SHA.fullmatch(args.revision), 'exact source commit is required')
        observation = None
        if args.github:
            actual = subprocess.check_output(['git', '-C', str(args.root), 'rev-parse', 'HEAD'], text=True).strip()
            require(actual == args.revision, 'checkout differs from requested source revision')
            require(not subprocess.check_output(['git', '-C', str(args.root), 'status', '--porcelain', '--untracked-files=no'], text=True),
                    'live release review requires an unchanged tracked checkout')
            observation = observe_github(args.repository, args.revision, 102)
        report = evaluate(args.root, args.revision, args.rc_report, observation, args.github)
        require(not args.report.is_symlink(), 'report output cannot be a symlink')
        require(args.report.resolve() not in {args.root.resolve() / item['path'] for item in report['inputs']},
                'report cannot overwrite source evidence')
        require(args.rc_report is None or args.report.resolve() != args.rc_report.resolve(), 'report cannot overwrite RC evidence')
        args.report.parent.mkdir(parents=True, exist_ok=True)
        payload = (json.dumps(report, sort_keys=True, separators=(',', ':'), allow_nan=False) + '\n').encode('utf-8')
        pending = None
        try:
            with tempfile.NamedTemporaryFile(dir=args.report.parent, prefix='.closeout-', delete=False) as stream:
                pending = Path(stream.name)
                stream.write(payload)
            os.replace(pending, args.report)
        finally:
            if pending is not None:
                pending.unlink(missing_ok=True)
        print('CLOSEOUT decision=' + report['decision'] + ' blockers=' + str(len(report['blockers'])) + ' production_claim=false')
        return 3 if args.require_ga and not report['publication_authorized'] else 0
    except (ValueError, OSError, KeyError, TypeError, RecursionError, subprocess.SubprocessError) as error:
        print('release closeout error: ' + str(error), file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
