// RUN: %shorthand-opt %s --split-input-file --verify-diagnostics
module {
  // expected-error@+1 {{unknown backend policy: imaginary}}
  shorthand.model @m {backend = #shorthand.backend<"imaginary">}
}
// -----
module {
  // expected-error@+1 {{claims mode must be evidence_only}}
  shorthand.greenai_contract @c {evidence = #shorthand.evidence<"MQ4", "DQ4", "certified">}
}
// -----
module {
  // expected-error@+1 {{expected an unencoded, non-scalar static ranked tensor}}
  shorthand.model @m {signature = !shorthand.model<tensor<?xf32>, tensor<1xf32>>}
}
// -----
module {
  %t = shorthand.tensor {name = "input", value = dense<1.0> : tensor<1xf32>} : tensor<1xf32>
  // expected-error@+1 {{model must resolve to a shorthand.model symbol}}
  %r = shorthand.infer @missing(%t) : (tensor<1xf32>) -> tensor<1xf32>
}
