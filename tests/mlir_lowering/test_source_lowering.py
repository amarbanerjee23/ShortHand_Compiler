"""Mandatory source/SDK differential execution and deterministic rejection."""
from pathlib import Path
import os, shutil, subprocess, sys, tempfile
root, compiler, driver, probe = (Path(x).resolve() for x in sys.argv[1:5])
clang=os.environ.get("SHORTHAND_LLVM_CLANG","clang++")
lli=os.environ.get("SHORTHAND_LLVM_LLI","lli")
checks=0

def run(*args, success=True, stdin=""):
    global checks
    r=subprocess.run(list(map(str,args)),input=stdin,text=True,capture_output=True,timeout=60); checks+=1
    assert (r.returncode==0)==success and r.returncode>=0, (args,r.returncode,r.stderr)
    assert not any(x in r.stderr for x in ("AddressSanitizer","LeakSanitizer","runtime error:","PLEASE submit a bug report")),r.stderr
    return r

with tempfile.TemporaryDirectory(prefix="shorthand-lowering-") as tmp:
    work=Path(tmp)
    def execute(source, expected="", reference=False, stdin="", error=None):
        if reference:
            r=run(compiler,source,"run",stdin=stdin,success=not error)
            assert error in r.stderr if error else r.stdout==expected,r
        ir=work/"generated.ll"; run(compiler,source,"compile-mlir","--output",ir)
        assert "unrealized_conversion_cast" not in ir.read_text()
        r=run(lli,ir,success=not error,stdin=stdin)
        assert error in r.stderr if error else r.stdout==expected,r
        for level in ("-O0","-O2"):
            flags=["-fsanitize=address,undefined","-fno-omit-frame-pointer"] if os.environ.get("SHORTHAND_MLIR_SANITIZERS")=="ON" else []
            native=work/"native"; run(clang,ir,level,*flags,"-o",native); r=run(native,success=not error,stdin=stdin)
            assert error in r.stderr if error else r.stdout==expected,r
        text=work/"source.mlir"; run(compiler,source,"emit-mlir","--output",text); assert "loc(" in text.read_text()
        lowered=work/"lowered.mlir"; run(driver,text,"--lower-shorthand-to-llvm","--canonicalize","--cse","-o",lowered)
        assert "unrealized_conversion_cast" not in lowered.read_text()
    for path in ("tests/semantic/differential/core_control","tests/semantic/differential/production_types","tests/semantic/functions_control/functions_control"):
        execute(root/(path+".short"),(root/(path+".expected")).read_text(),reference=True)
    project=work/"project"; shutil.copytree(root/"tests/modules/resolver/valid_project",project)
    for name in ("native_app.short","typed_app.short"):
        source=project/"src"/name; run(compiler,source,"lock"); execute(source,run(compiler,source,"run").stdout,reference=True)
    source=work/"io.short"; source.write_text('int value; float score; read value, score; print value, score;\n'); execute(source,"7 1.25\n",reference=True,stdin="7 1.25\n")
    execute(root/"tests/mlir_lowering/composites.short",(root/"tests/mlir_lowering/composites.expected").read_text())
    errors={"division_by_zero":"SHD7001","array_bounds":"SHD7002","float_division_by_zero":"SHD7001","float_array_bounds":"SHD7002"}
    for name,code in errors.items(): execute(root/f"tests/semantic/differential/{name}.short",reference=True,error=code)
    invalid=[p for p in (root/"tests/semantic/differential").glob("*.short") if p.stem not in {"core_control","production_types","goto_rejected",*errors}]
    invalid += [p for p in (root/"tests/semantic/functions_control").glob("*.short") if p.stem!="functions_control"]
    for source in invalid:
        for mode in ("emit-mlir","compile-mlir"):
            out=work/"preserve"; out.write_text("preserve existing artifact"); r=run(compiler,source,mode,"--output",out,success=False)
            assert "SHD" in r.stderr and out.read_text()=="preserve existing artifact",r
    header="language shorthand.enterprise_language.v2;\nnamespace test.values;\n"
    prefix="record R { int32 value; }; owned R original = R(1); "
    rejected={
        "moved-read":prefix+"move original to next; print original.value;",
        "borrowed-move":prefix+"borrow shared original as reader; move original to next; release reader;",
        "mutable-conflict":prefix+"borrow mutable original as writer; borrow shared original as reader;",
        "shared-write":prefix+"borrow shared original as reader; set reader.value = 2; release reader;",
        "released-handle":prefix+"borrow shared original as reader; release reader; print reader.value;",
        "owned-copy":prefix+"owned R copy = original;",
        "wrong-field":prefix+'set original.value = "wrong";',
        "unknown-field":prefix+"print original.missing;",
        "duplicate-field":"record R { int32 value; bool value; };",
        "wrong-payload":"option O<int32>; owned O value = O.some(2.0);",
        "borrowed-array":"slice V<int32>; array int32 data[1] = [1]; view V part = data[0, 1]; set data[0] = 2; release part;",
        "released-view":"slice V<int32>; array int32 data[1] = [1]; view V part = data[0, 1]; release part; print part[0];",
        "bad-string":'owned string value = "unterminated;', "truncated":"record R { int32",
        "depth":"record R { int32 value; }; owned R value = "+"R("*150+"1"+")"*150+";",
    }
    for name,body in rejected.items():
        source=work/(name+".short"); source.write_text(header+body); r=run(compiler,source,"compile-mlir",success=False)
        assert "SHD6002" in r.stderr and not r.stdout,r
    for name,body,code in (
        ("option","option O<int32>; owned O value = O.none; print value.value;","SHD7031"),
        ("result",'result R<int32,string>; owned R value = R.error("bad"); print value.ok;',"SHD7031"),
        ("range","slice V<int32>; array int32 data[1] = [1]; view V part = data[1, 1]; release part;","SHD7002"),
        ("index","slice V<int32>; array int32 data[1] = [1]; view V part = data[0, 1]; print part[-1]; release part;","SHD7002")):
        source=work/(name+".short"); source.write_text(header+body); execute(source,error=code)
    for name in ("api","bad-tag","inactive","missing-return"):
        for level in ("O0","O2"):
            ir=work/"api.ll"; run(probe,name,ir,level); run(clang,ir,"-"+level,"-o",work/"api"); r=run(work/"api",success=name=="api")
            assert r.stdout=="42\n" if name=="api" else ("SHD7004" if name=="missing-return" else "SHD7031") in r.stderr
print(f"PASS source, modules, composites, optimization and deterministic failures ({checks} commands)")
