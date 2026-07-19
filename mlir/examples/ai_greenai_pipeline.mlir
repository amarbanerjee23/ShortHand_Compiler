// Example-only ShortHand MLIR module.
// This is a scaffold artifact used by static validation until MLIR build support
// is introduced.

module {
  "shorthand.model"() {
    name = "classifier",
    format = "onnx",
    path = "models/classifier.onnx",
    task = "classification",
    precision = "float",
    input_shape = "1,4",
    output_shape = "1,2",
    backend_preference = "onnxruntime_cpu,fallback"
  } : () -> ()

  "shorthand.tensor"() {
    name = "input",
    element_type = "float",
    shape = "1,4",
    rank = 2 : i64,
    total_elements = 4 : i64
  } : () -> ()

  "shorthand.tensor"() {
    name = "output",
    element_type = "float",
    shape = "1,2",
    rank = 2 : i64,
    total_elements = 2 : i64
  } : () -> ()

  "shorthand.greenai_contract"() {
    name = "classifier_workload",
    functional_unit = "1 inference",
    success_criteria = "quality guardrail preserved",
    boundary = "compute",
    measurement_quality = "MQ1",
    data_quality = "DQ1",
    carbon_factor = 171.09 : f64,
    claims_mode = "evidence_only"
  } : () -> ()

  "shorthand.greenai_measure"() {
    workload = "classifier_workload",
    backend = "onnxruntime_cpu",
    inferences = 1 : i64,
    watts = 10.0 : f64,
    seconds = 0.1 : f64
  } : () -> ()

  "shorthand.infer"() {
    model = "classifier",
    input = "input",
    output = "output"
  } : () -> ()
}
