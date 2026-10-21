#!/usr/bin/env python3
"""Capture/replay bounded AOT vs prepared ORT under the same C++ host boundary."""
from __future__ import annotations
import argparse
import hashlib
import itertools
import json
import math
import pathlib
import random
import shutil
import statistics
import subprocess
import sys
import time

import compile_model

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'experiments/energy'))
from ci_energy_evidence import ComponentEnergySampler, cpu_identity

RUNNERS = ('aot', 'ort_basic', 'ort_all')
BATCHES = (1, 16, 32)
SEED = 124


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False)+'\n')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def orders():
    result = list(itertools.permutations(RUNNERS))
    random.Random(SEED).shuffle(result)
    return result


def command(argv, directory, name):
    write(directory/(name+'.command.json'), list(map(str, argv)))
    with (directory/(name+'.stdout')).open('w') as stdout, (directory/(name+'.stderr')).open('w') as stderr:
        start = time.perf_counter()
        proc = subprocess.run(list(map(str, argv)), stdout=stdout, stderr=stderr, timeout=240, check=False)
    if proc.returncode:
        raise ValueError(f'{name}: process failed; see retained stderr')
    return time.perf_counter()-start


def validate_sample(s, runner, batch, iterations):
    compile_model.require(isinstance(s, dict), 'sample object required')
    expected = dict(batch=batch, inputs=64, outputs=10, iterations=iterations, warmups=8,
                    completed_calls=iterations+9, verified_vectors=(iterations+9)*batch)
    compile_model.require(all(type(s.get(k)) is int and s[k] == v for k,v in expected.items()), 'incomplete/wrong workload')
    compile_model.require(s.get('schema') == 'shorthand.aot.sample.v1' and s.get('success') is True
                          and s.get('runner') == runner, 'wrong runner/sample schema')
    compile_model.require(all(type(s.get(k)) in (int,float) and math.isfinite(s[k]) and s[k] > 0
                          for k in ('cold_ms','elapsed_ms')), 'invalid sample timing')


def validate_energy(e,batch,iterations):
    compile_model.require(isinstance(e,dict), 'energy object required')
    count=(iterations+9)*batch
    elapsed=e.get('process_elapsed_seconds')
    compile_model.require(type(e.get('verified_vectors')) is int and e['verified_vectors']==count
        and type(elapsed) in (float,int) and math.isfinite(elapsed) and elapsed>0
        and type(e.get('samples')) is int and e['samples']>=0, 'invalid energy workload')
    compile_model.require(e.get('available') in (True,False) and type(e['available']) is bool
        and e.get('claim_authorized') is False and e.get('physical_system_energy_measured') is False,'invalid energy status')
    if not e['available']:
        compile_model.require(e.get('hardware_measured_joules') is None and e.get('joules_per_verified_vector') is None
            and e.get('evidence_class') is None and bool(e.get('reason')),'unavailable energy must be null')
    else:
        j=e.get('hardware_measured_joules'); count=(iterations+9)*batch
        compile_model.require(type(j) in (int,float) and math.isfinite(j) and j>0 and e.get('evidence_class')=='E1'
            and e['samples']>=2 and e.get('method') in ('rapl_powercap_package','amd_energy_hwmon_socket')
            and e.get('joules_per_verified_vector')==j/count,'invalid metered energy')


def metered(argv, cell, name, runner, batch, iterations):
    # Separate from latency samples. Startup, fixture loading, warmup, verified
    # calls and teardown are all included in the process energy boundary.
    sampler, reason, joules, samples = None, 'unavailable', None, 0
    started = False
    try:
        sampler = ComponentEnergySampler()
        sampler.start()
        started = True
    except Exception as exc:
        reason = str(exc)
    try:
        elapsed = command(argv, cell, name)
    finally:
        if started:
            try:
                joules, samples = sampler.stop()
                reason = 'component_energy_not_process_attributed'
            except Exception as exc:
                reason, joules = str(exc), None
    s = json.loads((cell/(name+'.stdout')).read_text())
    validate_sample(s,runner,batch,iterations)
    available = type(joules) in (int,float) and math.isfinite(joules) and joules > 0 and samples >= 2
    if not available and joules is not None:
        reason = 'invalid_component_energy'
    record = dict(available=available, evidence_class='E1' if available else None,
                  hardware_measured_joules=joules if available else None,
                  joules_per_verified_vector=joules/s['verified_vectors'] if available else None,
                  verified_vectors=s['verified_vectors'], process_elapsed_seconds=elapsed,
                  method=sampler.method if sampler else None, samples=samples, reason=reason,
                  boundary='whole child process; CPU component counter, not process-attributed',
                  physical_system_energy_measured=False, claim_authorized=False)
    write(cell/(name+'.energy.json'),record)


def capture(args):
    import numpy as np
    import onnx
    from onnx import helper, numpy_helper
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    compile_model.require(not any(output.iterdir()), 'use a fresh output directory')
    sdk = args.sdk.resolve()
    diff=subprocess.check_output(['git','diff','HEAD'],cwd=ROOT)
    (output/'source.diff').write_bytes(diff)
    manifest = dict(schema='shorthand.aot.benchmark.v1', experimental=True, seed=SEED,
                    revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
                    working_tree_diff_sha256=hashlib.sha256(diff).hexdigest(),
                    hardware=cpu_identity(), iterations=args.iterations, cells=[], observations=[], energy=[],
                    compiler=subprocess.check_output([args.cxx,'--version'],text=True).splitlines()[0],
                    sdk_library_sha256=sha(sdk/'lib/libonnxruntime.so'),
                    onnx_build_version=onnx.__version__,
                    boundary='same prepared C++ host: finite input scan, invoke, numerical output oracle; one thread',
                    comparison_scope='AOT versus ORT backend experiment; no ShortHand source or language attribution',
                    latency_claim_authorized=False, energy_claim_authorized=False)
    for batch in BATCHES:
        cell = output/f'b{batch}-t1'; cell.mkdir()
        rng = np.random.default_rng(SEED)
        w = rng.uniform(-0.5,0.5,(64,10)).astype(np.float32)
        b = rng.uniform(-0.5,0.5,10).astype(np.float32)
        x = rng.uniform(-2,2,(batch,64)).astype(np.float32)
        expected = (x.astype(np.float64)@w.astype(np.float64)+b.astype(np.float64)).astype(np.float32)
        graph = helper.make_graph([helper.make_node('MatMul',['X','W'],['Z']),helper.make_node('Add',['Z','B'],['Y'])],
            'nonzero_affine', [helper.make_tensor_value_info('X',onnx.TensorProto.FLOAT,[batch,64])],
            [helper.make_tensor_value_info('Y',onnx.TensorProto.FLOAT,[batch,10])],
            [numpy_helper.from_array(w,'W'),numpy_helper.from_array(b,'B')])
        model = cell/'input.onnx'
        model.write_bytes(helper.make_model(graph,opset_imports=[helper.make_opsetid('',13)],ir_version=9).SerializeToString())
        start = time.perf_counter()
        compile_model.emit(model,cell/'artifact')
        generation_seconds = time.perf_counter()-start
        x.astype('<f4').tofile(cell/'input.bin'); expected.astype('<f4').tofile(cell/'expected.bin')
        shutil.copyfile(ROOT/'experiments/aot/probe.cpp',cell/'probe.cpp')
        flags=[args.cxx,'-std=c++17','-O3','-fno-fast-math','-ffp-contract=off','-Wall','-Wextra','-Wpedantic','-Werror','-pthread']
        compile_seconds=command([*flags,'-c',cell/'artifact/model.cpp','-o',cell/'artifact/model.o'],cell,'compile-model')
        binaries={}
        for runner in RUNNERS:
            binary=cell/runner
            argv=[*flags,'-DSHORTHAND_AOT='+str(int(runner=='aot')),'-DSHORTHAND_ORT_ALL='+str(int(runner=='ort_all')),cell/'probe.cpp']
            if runner=='aot':
                argv += ['-I'+str(cell/'artifact'),cell/'artifact/model.o']
            else:
                argv += ['-I'+str(sdk/'include'),'-L'+str(sdk/'lib'),'-Wl,-rpath,'+str(sdk/'lib'),'-lonnxruntime']
            command([*argv,'-o',binary],cell,'link-'+runner)
            binaries[runner]=binary
        manifest['cells'].append(dict(batch=batch,generation_seconds=generation_seconds,
            native_compile_seconds=compile_seconds,object_bytes=(cell/'artifact/model.o').stat().st_size,
            executable_bytes={r:p.stat().st_size for r,p in binaries.items()},workspace_bytes=batch*10*4))
        for block,order in enumerate(orders()):
            for position,runner in enumerate(order):
                name=f'latency-{block}-{position}-{runner}'
                argv=[binaries[runner],model,cell/'input.bin',cell/'expected.bin',batch,64,10,args.iterations]
                command(argv,cell,name)
                sample=json.loads((cell/(name+'.stdout')).read_text())
                validate_sample(sample,runner,batch,args.iterations)
                manifest['observations'].append(dict(batch=batch,block=block,position=position,runner=runner,path=f'{cell.name}/{name}.stdout'))
        for runner in RUNNERS:
            name='energy-'+runner
            metered([binaries[runner],model,cell/'input.bin',cell/'expected.bin',batch,64,10,args.iterations],cell,name,runner,batch,args.iterations)
            manifest['energy'].append(dict(batch=batch,runner=runner,path=f'{cell.name}/{name}.energy.json'))
    manifest['hashes']={str(p.relative_to(output)):sha(p) for p in sorted(output.rglob('*')) if p.is_file()}
    write(output/'manifest.json',manifest)
    digest=sha(output/'manifest.json'); (output/'manifest.sha256').write_text(digest+'\n')
    return replay(output,digest)


def replay(output,digest):
    compile_model.require(sha(output/'manifest.json')==digest,'manifest digest mismatch')
    m=json.loads((output/'manifest.json').read_text())
    compile_model.require(m.get('schema')=='shorthand.aot.benchmark.v1' and m.get('seed')==SEED
        and m.get('latency_claim_authorized') is False and m.get('energy_claim_authorized') is False,'invalid manifest')
    iterations=m.get('iterations')
    compile_model.require(type(iterations) is int and 1<=iterations<=65536,'invalid iterations')
    hashes=m['hashes']
    required={'source.diff'}
    for batch in BATCHES:
        prefix=f'b{batch}-t1/'
        required.update(prefix+n for n in ('input.onnx','input.bin','expected.bin','probe.cpp',
            'artifact/model.onnx','artifact/model.cpp','artifact/model.h','artifact/model.o','artifact/manifest.json',*RUNNERS))
        stems=['compile-model',*('link-'+r for r in RUNNERS),*('energy-'+r for r in RUNNERS)]
        stems += [f'latency-{b}-{p}-{r}' for b,order in enumerate(orders()) for p,r in enumerate(order)]
        required.update(prefix+stem+suffix for stem in stems for suffix in ('.command.json','.stdout','.stderr'))
        required.update(prefix+'energy-'+r+'.energy.json' for r in RUNNERS)
    compile_model.require(isinstance(hashes,dict) and required <= hashes.keys(),'missing source/binary/raw evidence')
    for name,expected in hashes.items():
        p=pathlib.PurePosixPath(name)
        compile_model.require(not p.is_absolute() and '..' not in p.parts and '\\' not in name,'unsafe evidence path')
        path=output/name
        compile_model.require(not path.is_symlink() and output.resolve() in path.resolve().parents and sha(path)==expected,'artifact hash mismatch')
    compile_model.require(sha(output/'source.diff')==m.get('working_tree_diff_sha256'),'source diff digest mismatch')
    expected=[(batch,block,pos,runner) for batch in BATCHES for block,order in enumerate(orders()) for pos,runner in enumerate(order)]
    rows=m['observations']
    compile_model.require(len(rows)==len(expected),'incomplete observation matrix')
    seen=set(); values={}
    for row,key in zip(rows,expected):
        batch,block,pos,runner=key
        compile_model.require(tuple(row.get(k) for k in ('batch','block','position','runner'))==key
            and all(type(row[k]) is int for k in ('batch','block','position')),'wrong observation order')
        name=row['path']
        compile_model.require(name in hashes and name not in seen,'unhashed/duplicate observation')
        seen.add(name)
        sample=json.loads((output/name).read_text()); validate_sample(sample,runner,batch,iterations)
        values[key]=sample['elapsed_ms']/iterations/batch
    energy=m['energy']
    compile_model.require(len(energy)==len(BATCHES)*len(RUNNERS),'missing energy observations')
    for row,(batch,runner) in zip(energy,itertools.product(BATCHES,RUNNERS)):
        name=row['path']
        compile_model.require(type(row['batch']) is int and row['batch']==batch and row['runner']==runner
            and name in hashes and name not in seen,'wrong/duplicate energy cell')
        seen.add(name); e=json.loads((output/name).read_text())
        validate_energy(e,batch,iterations)
    result=[]
    for batch in BATCHES:
        by_block=[{r:values[(batch,b,order.index(r),r)] for r in RUNNERS} for b,order in enumerate(orders())]
        result.append(dict(batch=batch,median_ms_per_vector={r:statistics.median(v[r] for v in by_block) for r in RUNNERS},
            paired_ratios={r:dict(median=statistics.median(v['aot']/v[r] for v in by_block),
                minimum=min(v['aot']/v[r] for v in by_block),maximum=max(v['aot']/v[r] for v in by_block)) for r in RUNNERS[1:]}))
    report=dict(schema='shorthand.aot.report.v1',success=True,manifest_sha256=digest,rows=result,
                energy_availability=[dict(batch=e['batch'],runner=e['runner'],**json.loads((output/e['path']).read_text())) for e in energy],
                latency_claim_authorized=False,energy_claim_authorized=False)
    lines=['# Bounded CPU AOT observations','',m['comparison_scope']+'.', '',
        'Static FP32 MatMul+Add, deterministic nonzero input, batches 1/16/32, one thread. Both backends consume the same buffers and verify every output against a float64 NumPy oracle rounded to FP32 (atol=1e-5, rtol=1e-4). Six balanced three-runner permutations; every observed pair is retained. No shared-runner superiority claim is authorized.','',
        '| Batch | AOT ms/vector | ORT BASIC ms/vector | ORT ALL ms/vector | Paired AOT/BASIC median (range) | Paired AOT/ALL median (range) |',
        '|---|---:|---:|---:|---|---|']
    for row in result:
        times=' | '.join(f"{row['median_ms_per_vector'][r]:.6f}" for r in RUNNERS)
        ratios=' | '.join(f"{row['paired_ratios'][r]['median']:.4f} ({row['paired_ratios'][r]['minimum']:.4f}–{row['paired_ratios'][r]['maximum']:.4f})" for r in RUNNERS[1:])
        lines.append(f"| {row['batch']} | {times} | {ratios} |")
    lines += ['', 'Ratios below 1 mean lower observed AOT time in this boundary. AOT is a backend candidate compiled through C++17, not evidence that ShortHand beats C++. No ShortHand+AOT same-artifact host comparison is implemented yet.', '',
        'Energy is a separate whole-process component-counter pass, including setup, warmup, all verified calls and teardown. Missing counters or ambiguous wraps produce unavailable/null energy. No physical-system energy, calibrated estimate, carbon or certification claim is made.', '',
        '| Batch | Code generation seconds | Native compile seconds | Object bytes | Explicit workspace bytes |', '|---|---:|---:|---:|---:|']
    compile_model.require([c.get('batch') for c in m['cells']]==list(BATCHES),'invalid compile cells')
    for c in m['cells']:
        compile_model.require(type(c['batch']) is int and type(c['object_bytes']) is int and c['object_bytes']>0
            and type(c['workspace_bytes']) is int and c['workspace_bytes']==c['batch']*10*4
            and all(type(c.get(k)) in (float,int) and math.isfinite(c[k]) and c[k]>0
                    for k in ('generation_seconds','native_compile_seconds')), 'invalid compile metadata')
        cell=output/f"b{c['batch']}-t1"
        compile_model.require(c['object_bytes']==(cell/'artifact/model.o').stat().st_size
            and c.get('executable_bytes')=={r:(cell/r).stat().st_size for r in RUNNERS}, 'incorrect artifact sizes')
        lines.append(f"| {c['batch']} | {c['generation_seconds']:.6f} | {c['native_compile_seconds']:.6f} | {c['object_bytes']} | {c['workspace_bytes']} |")
    lines += ['', 'Compilation cost is excluded from resident times. Workspace bytes are the explicit caller buffer size, not a measured whole-process memory or allocation count. Detailed energy status, cold timing, compiler identity, raw outputs, commands and digests are retained in this bundle.','']
    # Validate the entire bundle before replacing any accepted derivative.
    write(output/'report.json',report)
    (output/'AOT.md').write_text('\n'.join(lines))
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    parser.add_argument('--sdk',type=pathlib.Path)
    parser.add_argument('--cxx',default='clang++-18')
    parser.add_argument('--iterations',type=int,default=1024)
    parser.add_argument('--replay-sha256')
    args=parser.parse_args()
    compile_model.require(1<=args.iterations<=65536,'iteration limit')
    if args.replay_sha256:
        replay(args.output.resolve(),args.replay_sha256)
    else:
        compile_model.require(args.sdk is not None,'SDK required for capture')
        capture(args)


if __name__=='__main__':
    main()
