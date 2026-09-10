"""Exercise the public parser/verifier, requiring each intended diagnostic."""
import subprocess
import sys

driver, example = sys.argv[1:]
source = open(example, encoding="utf-8").read()
cases = []


def reject(name, old, new, diagnostic):
    if old not in source:
        raise AssertionError(f"missing fixture anchor: {name}")
    cases.append((name, source.replace(old, new, 1), diagnostic))


for field, value in [("path", "models/classifier.onnx"), ("task", "classification"),
                     ("quality_guardrail", "accuracy >= 0.95")]:
    reject(f"empty model {field}", f'{field} = "{value}"', f'{field} = " "', "must be nonempty")
reject("missing model path", 'path = "models/classifier.onnx",', "", "requires attribute 'path'")
reject("unknown format", 'format = "onnx"', 'format = "unknown"', "unknown model format")
reject("incompatible backend", 'backend<"onnxruntime_cpu">', 'backend<"libtorch">', "incompatible with model format")
reject("unknown backend", 'backend<"onnxruntime_cpu">', 'backend<"gpu">', "unknown backend policy")
for shape, diagnostic in [
    ("?xf32", "static ranked tensor"), ("0xf32", "dimensions must be positive"),
    ("f32", "static ranked tensor"), ("1xf64", "unsupported tensor element type"),
    ("1xi1", "unsupported tensor element type"), ("1xsi8", "unsupported tensor element type"),
    ("9223372036854775807x2xi8", "element count overflows"),
    ("4611686018427387904xf32", "byte size overflows"),
]:
    reject("model shape " + shape, "!shorthand.model<tensor<1x4xf32>", f"!shorthand.model<tensor<{shape}>", diagnostic)
reject("output shape invalid", "tensor<1x2xf32>>", "tensor<?xf32>>", "static ranked tensor")
reject("wrong signature type", "!shorthand.model<tensor<1x4xf32>, tensor<1x2xf32>>", "tensor<1xf32>", "failed to satisfy constraint")
reject("data mismatch", "dense<1.0> : tensor<1x4xf32>", "dense<1.0> : tensor<1x2xf32>", "dense tensor data must match")
reject("empty tensor name", 'name = "input"', 'name = ""', "tensor name must be nonempty")
reject("missing tensor data", 'name = "input", value = dense<1.0> : tensor<1x4xf32>', 'name = "input"', "requires attribute 'value'")
reject("undefined model", "model = @classifier", "model = @missing", "model must resolve")
reject("wrong model symbol kind", "model = @classifier", "model = @classifier_workload", "model must resolve")
reject("wrong input signature", "!shorthand.model<tensor<1x4xf32>", "!shorthand.model<tensor<1x5xf32>", "exactly match the model signature")
reject("wrong output signature", "tensor<1x2xf32>>", "tensor<1x3xf32>>", "exactly match the model signature")
reject("undefined SSA", '"shorthand.infer"(%input)', '"shorthand.infer"(%missing)', "undeclared SSA value")
reject("duplicate symbols", 'sym_name = "classifier_workload"', 'sym_name = "classifier"', "redefinition of symbol")
reject("undefined contract", "workload = @classifier_workload", "workload = @missing", "workload must resolve")
reject("wrong contract symbol kind", "workload = @classifier_workload", "workload = @classifier", "workload must resolve")
for mq in ["MQ5", "MQ-1", "mq1", ""]:
    reject("bad MQ " + mq, '"MQ1"', f'"{mq}"', "measurement quality must be")
for dq in ["DQ5", "DQ-1", "dq1", ""]:
    reject("bad DQ " + dq, '"DQ1"', f'"{dq}"', "data quality must be")
for mode in ["certified", "carbon_neutral", "lowest_carbon", ""]:
    reject("bad claims " + mode, '"evidence_only"', f'"{mode}"', "claims mode must be evidence_only")
reject("empty boundary", '["compute", "memory"]', "[]", "at least one component")
for boundary in ['["compute", "compute"]', '["compute", ""]', '[" compute"]']:
    reject("bad boundary " + boundary, '["compute", "memory"]', boundary, "trimmed and unique")
reject("bad boundary type", '["compute", "memory"]', '[1 : i64]', "failed to satisfy constraint")
reject("empty functional unit", 'functional_unit = "1 successful inference"', 'functional_unit = " "', "must be nonempty")
reject("empty criteria", 'success_criteria = "quality and latency guardrails preserved"', 'success_criteria = ""', "must be nonempty")
for value in ["0.0", "-1.0", "0x7FF0000000000000", "0x7FF8000000000000"]:
    reject("invalid carbon factor " + value, "carbon_factor = 171.09", "carbon_factor = " + value, "finite and positive")
for field, original in [("energy_budget_j", "10.0"), ("carbon_budget_gco2e", "0.01")]:
    for value in ["-1.0", "0x7FF0000000000000", "0x7FF8000000000000"]:
        reject("invalid budget " + field + value, f"{field} = {original}", f"{field} = {value}", "finite and nonnegative")
for value in ["0", "-1", "-9223372036854775808"]:
    reject("invalid count " + value, "inferences = 1 : i64", f"inferences = {value} : i64", "finite and positive")
for field, original in [("watts", "10.0"), ("seconds", "0.1")]:
    for value in ["0.0", "-1.0", "0x7FF0000000000000", "0x7FF8000000000000"]:
        reject("invalid activity " + field + value, f"{field} = {original}", f"{field} = {value}", "finite and positive")
reject("joule overflow", "seconds = 0.1", "seconds = 1.0e308", "computed joules")
underflow = source.replace("watts = 10.0", "watts = 1.0e-300").replace("seconds = 0.1", "seconds = 1.0e-300")
cases.append(("joule underflow", underflow, "computed joules"))
reject("unregistered operation", '"shorthand.tensor"', '"shorthand.fake"', "unregistered operation")

for name, module, diagnostic in cases:
    result = subprocess.run([driver], input=module, text=True, capture_output=True, timeout=15)
    if result.returncode <= 0 or diagnostic not in result.stderr:
        raise AssertionError(f"{name}: expected rejection containing {diagnostic!r}; return={result.returncode}\n{result.stderr}")

# Each inventory backend/format can be represented. This does not execute it.
accepted = 0
for backend, fmt in [("fallback", "onnx"), ("onnxruntime_cpu", "onnx"),
                     ("onnxruntime_cuda", "onnx"), ("onnxruntime_tensorrt", "onnx"),
                     ("tensorrt", "tensorrt_engine"), ("openvino", "openvino_ir"),
                     ("libtorch", "torchscript"), ("llamacpp", "gguf")]:
    module = source.replace('backend<"onnxruntime_cpu">', f'backend<"{backend}">').replace('format = "onnx"', f'format = "{fmt}"')
    subprocess.run([driver], input=module, text=True, check=True, stdout=subprocess.DEVNULL, timeout=15)
    accepted += 1
for element in ["f16", "bf16", "i4", "i8", "i32"]:
    module = source.replace("f32", element)
    if element.startswith("i"):
        module = module.replace("dense<1.0>", "dense<1>")
    subprocess.run([driver], input=module, text=True, check=True, stdout=subprocess.DEVNULL, timeout=15)
    accepted += 1
for quality in range(5):
    module = source.replace('"MQ1"', f'"MQ{quality}"').replace('"DQ1"', f'"DQ{quality}"')
    subprocess.run([driver], input=module, text=True, check=True, stdout=subprocess.DEVNULL, timeout=15)
    accepted += 1
print(f"PASS MLIR boundary tests: {len(cases)} rejected, {accepted} accepted")
