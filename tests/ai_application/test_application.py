"""Mandatory real data, host boundary, serving recovery and negative tests."""
import copy
import hashlib
import json
import math
import os
import pathlib
import subprocess
import sys

root, tool, work = map(pathlib.Path, sys.argv[1:4])
live = sys.argv[4] == '1'
sys.path.insert(0, str(root / 'scripts'))
from create_digit_application_fixture import create, sha, write_json
from compare_ai_application_baselines import validate_pair

work = work / 'application'
config = create(work)
original = json.loads(config.read_text())
qpath = pathlib.Path(original['qualification_config'])
original_q = json.loads(qpath.read_text())
checks = 0

def run(*args, good=True, contains='', stdin=None):
    global checks
    result = subprocess.run([str(tool), *map(str, args)], input=stdin, text=True, capture_output=True, timeout=120,
                            env=dict(os.environ, ORT_DISABLE_TELEMETRY='1'))
    checks += 1
    assert result.returncode == (0 if good else 2), (args, result.returncode, result.stderr)
    assert not any(s in result.stderr for s in ('AddressSanitizer', 'LeakSanitizer', 'runtime error:')), result.stderr
    assert contains in result.stderr, result.stderr
    return result

def reset(app=None, qualification=None):
    q = copy.deepcopy(original_q if qualification is None else qualification)
    write_json(qpath, q)
    c = copy.deepcopy(original if app is None else app)
    c['qualification_sha256'] = sha(qpath)
    write_json(config, c)
    return c

run('application-describe', config, work / 'describe.json')
for key, value in [('unknown', 1), ('features', 63), ('minimum_accuracy', 0), ('minimum_accuracy', 1.1),
                   ('input_max', 0), ('scale', 0), ('top_k', 11), ('threads', 0), ('workers', 0), ('workers', 17),
                   ('dataset_sha256', '0'), ('request_timeout_ms', 0), ('dataset_source', '')]:
    c = copy.deepcopy(original); c[key] = value; reset(c)
    run('application', config, work / 'bad.json', good=False)
reset()
config.write_text(config.read_text().replace('"features": 64,', '"features":64,"features":64,'))
run('application', config, work / 'bad.json', good=False)
reset()
c = copy.deepcopy(original); c['qualification_sha256'] = '0' * 64; write_json(config, c)
run('application', config, work / 'bad.json', good=False, contains='sha256_mismatch')
reset()
c = copy.deepcopy(original); c['dataset_sha256'] = '0' * 64; reset(c)
run('application', config, work / 'bad.json', good=False, contains='sha256_mismatch')

original_data = pathlib.Path(original['dataset_path']).read_text()
for name, data in [('nan', original_data.replace('0,', 'nan,', 1)), ('range', original_data.replace('0,', '17,', 1)),
                   ('columns', original_data.replace(',', '', 1)), ('empty', ''), ('one_class', '0,' * 64 + '0\n'),
                   ('trailing', original_data.splitlines()[0] + ',\n'), ('bad_label', '0,' * 64 + '10\n'),
                   ('bounded_report', original_data * 20)]:
    path = work / (name + '.csv'); path.write_text(data)
    c = copy.deepcopy(original); c.update(dataset_path=str(path), dataset_sha256=sha(path)); reset(c)
    run('application', config, work / 'bad.json', good=False)
reset()
if live:
    report_path = work / 'native.json'
    run('application', config, report_path)
    native = json.loads(report_path.read_text())
    assert native['success'] and native['rows'] == 1797 and native['accuracy'] >= .85
    assert len(native['predictions']) == 1797 and sum(native['confusion_matrix_row_major']) == 1797
    assert native['top_k_accuracy'] >= native['accuracy'] and not native['comparative_energy_claim']
    assert all(t['completed'] == 3594 and not t['energy']['claim_eligible'] for t in native['trials'])
    assert len(native['trials']) == 3 and len(native['scores']) == 17970
    run('application-serve', config, work / 'serving.json')
    served = json.loads((work / 'serving.json').read_text())
    assert served['predictions'] == native['predictions'] and served['success']
    for batch in (1, 32):
        q = copy.deepcopy(original_q)
        q.update(batch_size=batch, input_shape=[batch, 64], output_shape=[batch, 10])
        reset(qualification=q); run('application', config, work / f'batch-{batch}.json')
        result = json.loads((work / f'batch-{batch}.json').read_text())
        assert result['predictions'] == native['predictions'] and result['rows'] == 1797
        assert len(result['scores']) == len(native['scores'])
        assert all(math.isclose(a, b, rel_tol=1e-4, abs_tol=1e-5)
                   for a, b in zip(result['scores'], native['scores']))
    reset()
    c = copy.deepcopy(original); c['minimum_accuracy'] = .99; reset(c)
    run('application', config, work / 'quality-failure.json', good=False)
    assert json.loads((work / 'quality-failure.json').read_text())['reason'] == 'application_accuracy_below_threshold'
    q = copy.deepcopy(original_q); q['maximum_latency_ms'] = 1e-12; reset(qualification=q)
    run('application', config, work / 'latency-failure.json', good=False)
    assert json.loads((work / 'latency-failure.json').read_text())['reason'] == 'application_p95_latency_limit'
    q = copy.deepcopy(original_q); q['require_measured_energy'] = True; reset(qualification=q)
    run('application', config, work / 'energy-failure.json', good=False)
    reset()
    raw = list(map(int, original_data.splitlines()[0].split(',')))[:-1]
    def frame(requests): return json.dumps({'requests': requests})
    def req(name, values=raw, tenant='application'):
        return dict(id=name, tenant=tenant, values=values, timeout_ms=30000)
    frames = [frame([req('good')]), frame([req('isolation', tenant='other')]), frame([req('malformed', values=[1])]),
              frame([req('recovery')]), frame([req('outside', values=[17] * 64)]),
              '{"requests":[],"model_path":"untrusted.onnx"}', 'x' * (1024 * 1024 + 1), frame([req('after-size')]),
              frame([req('duplicate'), req('duplicate')]), frame([req('last')])]
    output = run('application-stream', config, stdin='\n'.join(frames) + '\n')
    replies = [json.loads(line) for line in output.stdout.splitlines()]
    assert len(replies) == len(frames)
    assert replies[0]['results'][0]['classification']['predictions'] == [native['predictions'][0]]
    assert not replies[1]['results'][0]['success'] and 'isolation' in replies[1]['results'][0]['admission']
    assert not replies[2]['results'][0]['success'] and replies[3]['results'][0]['success']
    assert not replies[4]['results'][0]['success'] and 'error' in replies[5] and 'error' in replies[6]
    assert replies[7]['results'][0]['success'] and not replies[8]['results'][1]['success'] and replies[9]['results'][0]['success']
    # Full, partial and final batches must transfer only real rows; repeated
    # requests must not retain stale scores from an earlier batch.
    rows = [list(map(float, line.split(',')))[:-1] for line in original_data.splitlines()]
    batch = original_q['batch_size']
    cases = [(0, batch), (0, 1), (len(rows) - 1, 1), (0, batch)]
    batch_frames = [frame([req(f'batch-{i}', values=[v for row in rows[start:start+count] for v in row])])
                    for i, (start, count) in enumerate(cases)]
    batch_reply = run('application-stream', config, stdin='\n'.join(batch_frames) + '\n')
    batch_results = [json.loads(line) for line in batch_reply.stdout.splitlines()]
    assert len(batch_results) == len(cases)
    for reply, (start, count) in zip(batch_results, cases):
        item = reply['results'][0]
        assert item['success']
        classification = item['classification']
        assert classification['predictions'] == native['predictions'][start:start+count]
        expected = native['scores'][start*10:(start+count)*10]
        assert len(classification['scores']) == len(expected)
        assert all(math.isclose(a, b, rel_tol=1e-4, abs_tol=1e-5)
                   for a, b in zip(classification['scores'], expected))
        top_k = []
        for row in range(count):
            scores = classification['scores'][row*10:(row+1)*10]
            top_k.extend(sorted(range(10), key=lambda label: (-scores[label], label))[:original['top_k']])
        assert classification['top_k'] == top_k
    # Compare identical copies only to exercise validators, never as measured evidence.
    python = copy.deepcopy(native)
    assert not validate_pair(native, python, original_q)
    for key, value in [('backend_version', 'wrong'), ('batch_size', 999), ('dataset_sha256', '0' * 64),
                       ('hardware_fingerprint', 'other'), ('accuracy', 0), ('trials', [])]:
        bad = copy.deepcopy(python); bad[key] = value
        try: validate_pair(native, bad, original_q)
        except ValueError: pass
        else: raise AssertionError('comparison accepted ' + key)
        checks += 1
    bad = copy.deepcopy(python); bad['scores'][0] += 1
    try: validate_pair(native, bad, original_q)
    except ValueError: pass
    else: raise AssertionError('numeric mismatch accepted')
    try: validate_pair(native, python, original_q, True)
    except ValueError: pass
    else: raise AssertionError('unavailable energy accepted')
else:
    run('application', config, work / 'sdk-off.json', good=False, contains='application_prepare_failed')
    run('application-stream', config, good=False, contains='application_prepare_failed')
reset()
print(f'PASS {checks} application boundary cases; real_dataset_rows=1797; live_onnx={int(live)}; no measured energy claim')
