"""Mandatory real ONNX CPU execution through generated code and the frozen ABI."""
from pathlib import Path
import base64, os, subprocess, sys, tempfile
root,build,probe,sdk=(Path(x).resolve() for x in sys.argv[1:5])
assert (sdk/"include/onnxruntime_cxx_api.h").is_file(),"real ONNX SDK is mandatory"
clang=os.environ.get("SHORTHAND_LLVM_CLANG","clang++")
env=dict(os.environ); env["LD_LIBRARY_PATH"]=str(sdk/"lib")+":"+env.get("LD_LIBRARY_PATH","")
flags=["-fsanitize=address,undefined","-fno-omit-frame-pointer"] if os.environ.get("SHORTHAND_MLIR_SANITIZERS")=="ON" else []
def run(*args,success=True):
    r=subprocess.run(list(map(str,args)),text=True,capture_output=True,env=env,timeout=90)
    assert (r.returncode==0)==success and r.returncode>=0,(args,r.returncode,r.stderr)
    assert not any(x in r.stderr for x in ("AddressSanitizer","LeakSanitizer","runtime error:")),r.stderr
    return r
with tempfile.TemporaryDirectory(prefix="shorthand-live-lowering-") as tmp:
    work=Path(tmp); model=work/"identity.onnx"; model.write_bytes(base64.b64decode((root/"tests/fixtures/onnx/identity_float32_v13.onnx.b64").read_text()))
    harness=work/"harness.cpp"; harness.write_text('''#include "runtime/ShorthandRuntime.h"
#include <cstring>
extern "C" int shorthand_entry();
int main() {
 if(short_runtime_reset()!=0 || shorthand_entry()!=0) return 1;
 if(short_runtime_infer_success_count()!=1 || short_runtime_measurement_count()!=1) return 2;
 if(std::strcmp(short_runtime_last_infer_backend(),"onnxruntime_cpu")) return 3;
 if(!std::strstr(short_runtime_infer_bridge_request_json(),"ai_runtime_execution_succeeded")) return 4;
 return 0;
}
''')
    ir=work/"ai.ll"; run(probe,"ai",ir,"O2",model)
    assert "@llvm.used" in ir.read_text() and "evidence_only" in ir.read_text()
    for level in ("-O0","-O2"):
        obj=work/"generated.o"; run(clang,ir,level,*flags,"-c","-o",obj); run("objcopy","--redefine-sym","main=shorthand_entry",obj)
        native=work/"ai"; run(clang,harness,obj,build/"libshorthand_runtime.a","-std=c++17",level,*flags,"-I",root/"Compiler_new_ws/Short_Hand/src","-L",sdk/"lib","-lonnxruntime","-pthread","-o",native)
        r=run(native); assert r.stdout=="42\n",r.stdout
        assert not any(x in r.stderr.lower() for x in ("fallback","not_executed","backend_not_available")),r.stderr
        assert "shorthand_ai_metadata" in run("readelf","-SW",native).stdout and b"evidence_only" in native.read_bytes()
    model.write_bytes(b"not an ONNX model"); r=run(native,success=False); assert "SHD7030" in r.stderr and not r.stdout
    model.unlink(); r=run(native,success=False); assert "SHD7030" in r.stderr and not r.stdout
    stub=work/"fault.cpp"; stub.write_text('''extern "C" {
int short_ai_register_model(const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*) { return 0; }
int short_ai_register_tensor(const char*,const char*,const char*,const char*,const char*) { return 0; }
int short_ai_infer_f32(const char*,const char*,const float*,int,const char*,float*,int,int*) { return 2; }
int short_greenai_register_contract(const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*) { return 0; }
int short_greenai_record_measurement(const char*,const char*,const char*,const char*,const char*) { return 0; }
}
''')
    run(clang,ir,stub,"-O2",*flags,"-o",work/"unavailable"); r=run(work/"unavailable",success=False); assert "SHD7030" in r.stderr and not r.stdout
    stub.write_text(stub.read_text().replace('int,int*) { return 2; }','int,int* count) { *count=0; return 0; }'))
    run(clang,ir,stub,"-O2",*flags,"-o",work/"bad-count"); r=run(work/"bad-count",success=False); assert "unexpected tensor size" in r.stderr and not r.stdout
print("PASS real ONNX CPU output 42, ABI telemetry, retained evidence and runtime failures")
