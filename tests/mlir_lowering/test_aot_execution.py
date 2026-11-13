"""Opt-in source AOT, same-object C++ parity and real hybrid validation."""
from pathlib import Path
import argparse
import hashlib
import os
import re
import subprocess
import sys
import tempfile
# Must precede ORT import: the API alone cannot suppress its initialization event.
os.environ['ORT_DISABLE_TELEMETRY']='1'
import numpy as np
import onnx
from onnx import helper, numpy_helper, TensorProto
import onnxruntime as ort
ort.disable_telemetry_events()
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ai_application'))
from test_native_aot_importer import affine

def main():
    p=argparse.ArgumentParser();p.add_argument('--build',type=Path,required=True);p.add_argument('--sdk',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');args=p.parse_args()
    build=args.build.resolve();sdk=args.sdk.resolve();compiler=build/'short_hand';cxx=os.environ.get('SHORTHAND_LLVM_CLANG','clang++');objcopy=os.environ.get('SHORTHAND_LLVM_OBJCOPY','llvm-objcopy')
    flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if args.sanitizers else []
    env=dict(os.environ,ORT_DISABLE_TELEMETRY='1',SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE='1');env['LD_LIBRARY_PATH']=str(sdk/'lib')+':'+env.get('LD_LIBRARY_PATH','')
    for key in ('SHORTHAND_DEVICE_OVERRIDE','SHORTHAND_DEVICE_DENY','SHORTHAND_DEVICE_PREFERENCE','SHORTHAND_ALLOW_CPU_FALLBACK','SHORTHAND_MIN_DEVICE_MEMORY_MB'):env.pop(key,None)
    checks=0
    with tempfile.TemporaryDirectory(prefix='shorthand-source-aot-') as temp:
        work=Path(temp)
        def run(*argv,ok=True,extra=None):
            nonlocal checks
            r=subprocess.run(list(map(str,argv)),cwd=work,env=env|dict(extra or {}),text=True,capture_output=True,timeout=180);checks+=1
            assert r.returncode>=0 and (r.returncode==0)==ok,(argv,r.returncode,r.stdout,r.stderr)
            assert not any(s in r.stderr for s in ('AddressSanitizer','LeakSanitizer','runtime error:','PLEASE submit a bug report')),r.stderr
            return r
        def source(m,k,n,values,op='affine',end='infer model_data(input) -> output;\nprint output[0];'):
            return f'''int guard;
model model_data {{format onnx; path "{op}.onnx"; task "aot"; precision float;
input_shape "{m},{k}"; output_shape "{m},{n}"; backend_preference onnxruntime_cpu;
compact false; quality_guardrail accuracy >= 1; }};
tensor input fp32 "{m},{k}" = [{','.join(format(float(v),'.9g') for v in values)}];
tensor output fp32 "{m},{n}";
{end}
'''
        def compile_source(text,mode='compile-aot',ok=True):
            src=work/'program.short';src.write_text(text);ir=work/'program.ll';ir.write_text('sentinel');r=run(compiler,src,mode,'--output',ir,ok=ok)
            if not ok:assert ir.read_text()=='sentinel' and 'SHD' in r.stderr
            return ir
        def link(ir,level,backend=False):
            exe=work/('program'+level);run(cxx,ir,'-O'+level,*flags,build/'libshorthand_aot_support.a',*([build/'libshorthand_runtime.a','-L',sdk/'lib','-lonnxruntime','-pthread'] if backend else []),'-o',exe);return exe
        def host(ir,level,data,overflow=False):
            obj=work/'program.o';run(cxx,'-c',ir,'-O'+level,*flags,'-o',obj);run(objcopy,'--redefine-sym','main=__sh_aot_source_main',obj);exe=work/'host'
            run(cxx,'-std=c++17','-O'+level,*flags,'-pthread','-DAOT_SYMBOL=__sh_aot_v1_'+hashlib.sha256(data).hexdigest(),ROOT/'tests/ai_application/native_aot_execution_probe.cpp',obj,build/'libshorthand_aot_support.a','-o',exe)
            r=run(exe,work/'input.bin',work/'expected.bin',*(['overflow'] if overflow else []));assert 'PASS finite overflow' in r.stdout if overflow else 'PASS same AOT artifact' in r.stdout
        for m,k,n in ((1,1,1),(3,5,7),(16,64,10),(32,64,10),(64,1,1024)):
            model=affine(m,k,n);weights=numpy_helper.to_array(model.graph.initializer[0]);bias=numpy_helper.to_array(model.graph.initializer[1]);x=np.random.default_rng(91).uniform(-2,2,(m,k)).astype(np.float32);expected=np.zeros((m,n),dtype=np.float32)
            for i in range(k):expected=np.float32(expected+np.float32(x[:,i:i+1]*weights[i]))
            expected=np.float32(expected+bias);actual=ort.InferenceSession(model.SerializeToString(),providers=['CPUExecutionProvider']).run(None,{'X':x})[0];np.testing.assert_allclose(actual,expected,rtol=2e-5,atol=2e-5)
            data=model.SerializeToString();path=work/'affine.onnx';path.write_bytes(data);ir=compile_source(source(m,k,n,x.flat));text=ir.read_text()
            assert '__sh_aot_v1_'+hashlib.sha256(data).hexdigest() in text and 'short_ai_infer' not in text and not re.search(r'\b(?:fmul|fadd)\s+(?:fast|contract|reassoc)\b',text)
            x.astype('<f4').tofile(work/'input.bin');expected.astype('<f4').tofile(work/'expected.bin')
            for level in ('0','2'):
                exe=link(ir,level);path.rename(work/'hidden-model');assert abs(float(run(exe).stdout.strip())-float(expected.flat[0]))<1e-4
                assert 'SHD7030' in run(exe,ok=False,extra={'SHORTHAND_DEVICE_DENY':'cpu'}).stderr
                host(ir,level,data);(work/'hidden-model').rename(path)
        small=affine(1,1,1);small.graph.initializer[0].CopyFrom(numpy_helper.from_array(np.array([[2]],dtype=np.float32),'W'));small.graph.initializer[1].CopyFrom(numpy_helper.from_array(np.array([1],dtype=np.float32),'B'));(work/'affine.onnx').write_bytes(small.SerializeToString())
        declarations=source(1,1,1,[1],end='').replace('int guard;\n','');text='int guard; float answer;\ndef float recurse(int depth;) {\n'+declarations+'''
          if depth>0 {answer=recurse(depth-1);}
          infer model_data(input) -> input; infer model_data(input) -> input; return input[0];
        }; answer=recurse(2); print answer;
        ''';ir=compile_source(text)
        for level in ('0','2'):assert run(link(ir,level)).stdout.strip()=='127'
        strict=affine(1,2,1);strict.graph.initializer[0].CopyFrom(numpy_helper.from_array(np.array([[1],[1+2**-23]],dtype=np.float32),'W'));strict.graph.initializer[1].CopyFrom(numpy_helper.from_array(np.array([0],dtype=np.float32),'B'));(work/'affine.onnx').write_bytes(strict.SerializeToString());ir=compile_source(source(1,2,1,[-(1+2**-22),1+2**-23]))
        for level in ('0','2'):assert float(run(link(ir,level)).stdout.strip())==0
        small.graph.initializer[0].CopyFrom(numpy_helper.from_array(np.array([[np.finfo(np.float32).max]],dtype=np.float32),'W'));data=small.SerializeToString();(work/'affine.onnx').write_bytes(data);ir=compile_source(source(1,1,1,[2]));np.array([2],dtype='<f4').tofile(work/'input.bin');np.array([999],dtype='<f4').tofile(work/'expected.bin')
        for level in ('0','2'):
            assert 'SHD7030' in run(link(ir,level),ok=False).stderr;host(ir,level,data,True)
        for op,valid in (('Identity',True),('NotAnOnnxOperator',False)):
            graph=helper.make_graph([helper.make_node(op,['X'],['Y'])],'hybrid',[helper.make_tensor_value_info('X',TensorProto.FLOAT,[1,1])],[helper.make_tensor_value_info('Y',TensorProto.FLOAT,[1,1])]);model=helper.make_model(graph,ir_version=8,opset_imports=[helper.make_opsetid('',13)]);(work/'other.onnx').write_bytes(model.SerializeToString());src=source(1,1,1,[42.5],'other')
            compile_source(src,ok=False);ir=compile_source(src,'compile-hybrid');assert 'short_ai_infer_f32' in ir.read_text()
            for level in ('0','2'):
                result=run(link(ir,level,True),ok=valid);assert float(result.stdout.strip())==42.5 if valid else 'SHD7030' in result.stderr
        # Instrument the native preparation unit itself, independently of the SDK.
        (work/'affine.onnx').write_bytes(affine(1,1,1).SerializeToString())
        graph=helper.make_graph([helper.make_node('Identity',['X'],['Y'])],'prepare',
            [helper.make_tensor_value_info('X',TensorProto.FLOAT,[1,1])],
            [helper.make_tensor_value_info('Y',TensorProto.FLOAT,[1,1])])
        unsupported=helper.make_model(graph,ir_version=8,opset_imports=[helper.make_opsetid('',13)])
        (work/'unsupported.onnx').write_bytes(unsupported.SerializeToString())
        src=ROOT/'Compiler_new_ws/Short_Hand/src';probe=work/'prepare'
        run(cxx,'-std=c++17','-O1',*flags,'-I'+str(src),'-I'+str(ROOT/'mlir/include'),
            ROOT/'tests/ai_application/native_aot_prepare_probe.cpp',src/'aot/PrepareAot.cpp',
            src/'aot/StaticAffineModel.cpp',src/'module/Sha256.cpp','-o',probe)
        assert 'PASS native AOT preparation:' in run(probe,work/'affine.onnx',work/'unsupported.onnx').stdout
        for data in (b'',b'bad protobuf',(work/'other.onnx').read_bytes()[:-1]):
            (work/'other.onnx').write_bytes(data)
            for mode in ('compile-aot','compile-hybrid'):compile_source(source(1,1,1,[1],'other'),mode,False)
        (work/'affine.onnx').write_bytes(affine(1,1,1).SerializeToString());compile_source(source(1,2,1,[1,2]),ok=False);compile_source(source(1,1,1,[1]).replace('onnxruntime_cpu','fallback'),ok=False);compile_source(source(1,1,1,[1],'missing'),'compile-mlir')
    print(f'PASS native source AOT: {checks} commands, exact FP32 C++ parity, source O0/O2, bounded workspace, hybrid rejection')
if __name__=='__main__':main()
