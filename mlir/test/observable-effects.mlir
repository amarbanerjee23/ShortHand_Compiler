// RUN: %shorthand-opt %s --canonicalize --cse | %FileCheck %s
// CHECK: shorthand.model @classifier
// CHECK: shorthand.greenai_contract @classifier_workload
// CHECK: shorthand.tensor
// CHECK: shorthand.infer @classifier
// CHECK: shorthand.greenai_measure @classifier_workload
// CHECK: shorthand.infer @classifier
// CHECK: shorthand.greenai_measure @classifier_workload
// Parsed and verified by the registered ShortHand dialect in mandatory CI.
// Declared activity is candidate metadata; this module does not execute a model.
module {
  "shorthand.model"() {
    sym_name = "classifier",
    signature = !shorthand.model<tensor<1x4xf32>, tensor<1x2xf32>>,
    format = "onnx", path = "models/classifier.onnx", task = "classification",
    quality_guardrail = "accuracy >= 0.95",
    backend = #shorthand.backend<"onnxruntime_cpu">
  } : () -> ()
  "shorthand.greenai_contract"() {
    sym_name = "classifier_workload", functional_unit = "1 successful inference",
    success_criteria = "quality and latency guardrails preserved",
    quality_guardrail = "accuracy >= 0.95", boundary = ["compute", "memory"],
    evidence = #shorthand.evidence<"MQ1", "DQ1", "evidence_only">,
    carbon_factor = 171.09 : f64,
    energy_budget_j = 10.0 : f64, carbon_budget_gco2e = 0.01 : f64
  } : () -> ()
  %input = "shorthand.tensor"() {
    name = "input", value = dense<1.0> : tensor<1x4xf32>
  } : () -> tensor<1x4xf32>
  %output = "shorthand.infer"(%input) {
    model = @classifier
  } : (tensor<1x4xf32>) -> tensor<1x2xf32> loc("workload.short":12:3)
  "shorthand.greenai_measure"() {
    workload = @classifier_workload, backend = #shorthand.backend<"onnxruntime_cpu">,
    inferences = 1 : i64, watts = 10.0 : f64, seconds = 0.1 : f64
  } : () -> ()
  %second = "shorthand.infer"(%input) {
    model = @classifier
  } : (tensor<1x4xf32>) -> tensor<1x2xf32> loc("workload.short":12:3)
  "shorthand.greenai_measure"() {
    workload = @classifier_workload, backend = #shorthand.backend<"onnxruntime_cpu">,
    inferences = 1 : i64, watts = 10.0 : f64, seconds = 0.1 : f64
  } : () -> ()
}
