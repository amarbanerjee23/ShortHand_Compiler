"""Conversion boundaries must reject malformed or unsupported IR before DCE."""
import subprocess
import sys

driver = sys.argv[1]
record = '!shorthand.value<"record", "Pair", [i32], ["count"]>'
body = f'''module {{ func.func @main() -> i32 {{
  %a = arith.constant 40 : i32
  %b = arith.constant 42 : i32
  %v = "shorthand.value"(%a) : (i32) -> {record}
  %u = "shorthand.update"(%v, %b) {{index = 0 : i64}} : ({record}, i32) -> {record}
  %r = "shorthand.project"(%u) {{index = 0 : i64}} : ({record}) -> i32
  return %r : i32
}} }}'''
model = '''"shorthand.model"() {sym_name="m", signature=!shorthand.model<tensor<1xf16>, tensor<1xf16>>,
 format="onnx", path="model.onnx", task="identity", quality_guardrail="exact",
 backend=#shorthand.backend<"onnxruntime_cpu">} : () -> ()'''
inference = '''module { MODEL func.func @main() {
 %t = "shorthand.tensor"() {name="input", value=dense<1.0> : tensor<1xf16>} : () -> tensor<1xf16>
 %r = "shorthand.infer"(%t) {model=@m} : (tensor<1xf16>) -> tensor<1xf16>
 return
} }'''.replace("MODEL", model)
oversize = '''module { func.func @main() {
 %t = "shorthand.tensor"() {name="oversize", value=dense<0.0> : tensor<65537xf32>} : () -> tensor<65537xf32>
 return
} }'''
cases = [
    (body.replace('index = 0', 'index = -1'), 'index'),
    (body.replace(record, record.replace('[i32]', '[f64]')), 'exact storage'),
    (body.replace('-> '+record+'\n  %r', '-> '+record.replace('Pair', 'Other')+'\n  %r').replace('project"(%u)', 'project"(%v)'), 'record identity'),
    (body.replace('[i32], ["count"]', '[i32, i32], ["count", "count"]'), 'unique'),
    (body.replace('"record"', '"unknown"'), 'unknown composite kind'),
    (body.replace('[i32]', '[tensor<1xf32>]'), 'composite payloads'),
    (body.replace('module {', 'module { llvm.func @exit()'), 'runtime ABI symbol conflict'),
    (body.replace('module {', 'module { llvm.func @exit(%code: i32) { llvm.return }'), 'runtime ABI symbol conflict'),
    (oversize, '65536'),
    (oversize.replace('func.func @main() {', '').replace(' return\n}', ''), 'function body'),
    (inference, 'requires float32'),
    (inference.replace('task="identity"', 'shorthand.backend_preference=["fallback", "onnxruntime_cpu"], task="identity"'), 'first backend preference'),
]
for optimized in (False, True):
    flags = ['--lower-shorthand-to-llvm'] + (['--canonicalize', '--cse'] if optimized else [])
    for source, diagnostic in [(body, None)] + cases:
        result = subprocess.run([driver, *flags], input=source, text=True, capture_output=True, timeout=30)
        assert result.returncode >= 0, result.stderr
        assert not any(x in result.stderr for x in ('AddressSanitizer', 'LeakSanitizer', 'runtime error:')), result.stderr
        if diagnostic is None:
            assert result.returncode == 0 and 'llvm.func @main' in result.stdout, result.stderr
            assert 'shorthand.' not in result.stdout and 'unrealized_conversion_cast' not in result.stdout, result.stdout
        else:
            assert result.returncode != 0 and diagnostic in result.stderr, (diagnostic, result.stderr)
print('PASS MLIR LLVM conversion, composite and runtime ABI rejection before optimization')
