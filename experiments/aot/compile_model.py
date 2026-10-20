#!/usr/bin/env python3
"""Experimental, opt-in ONNX MatMul+Add -> standalone CPU C++ artifact.

This is a bounded AOT feasibility tool, not the ShortHand source lowering path.
It never loads external tensor data or silently substitutes an SDK backend.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import pathlib

MAX_MODEL_BYTES = 16 * 1024 * 1024
MAX_ELEMENTS = 65536
MAX_WEIGHTS = 1024 * 1024


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def static_shape(value):
    import onnx
    require(value.type.WhichOneof('value') == 'tensor_type', 'tensor type required')
    tensor = value.type.tensor_type
    require(tensor.elem_type == onnx.TensorProto.FLOAT, 'only FP32 tensors are supported')
    require(tensor.HasField('shape'), 'static shape required')
    dims = tensor.shape.dim
    require(len(dims) == 2 and all(d.WhichOneof('value') == 'dim_value' and 0 < d.dim_value <= 1024 for d in dims),
            'only positive static rank-2 shapes bounded by 1024 are supported')
    shape = tuple(d.dim_value for d in dims)
    require(math.prod(shape) <= MAX_ELEMENTS, 'tensor element limit exceeded')
    return shape


def inspect_model(data):
    import numpy as np
    import onnx
    from onnx import numpy_helper
    require(0 < len(data) <= MAX_MODEL_BYTES, 'model byte limit exceeded')
    model = onnx.load_model_from_string(data)  # No external-data resolution.
    recognized = onnx.ModelProto()
    recognized.CopyFrom(model)
    recognized.DiscardUnknownFields()
    require(recognized.SerializeToString(deterministic=True) == model.SerializeToString(deterministic=True),
            'unknown protobuf fields are unsupported')
    require(8 <= model.ir_version <= 10, 'supported ONNX IR versions are 8 through 10')
    require(len(model.opset_import) == 1 and model.opset_import[0].domain == ''
            and model.opset_import[0].version == 13, 'only standard-domain opset 13 is supported')
    require(not model.functions and not model.training_info, 'functions/training graphs are unsupported')
    require(not model.configuration, 'multi-device configurations are unsupported')
    graph = model.graph
    require(len(graph.input) == len(graph.output) == 1, 'one input and one output required')
    require(not graph.sparse_initializer and not graph.quantization_annotation, 'sparse/quantized graphs are unsupported')
    require(len(graph.node) == 2 and len(graph.initializer) == 2, 'exactly MatMul+Add and two constants required')
    mm, add = graph.node
    require(mm.op_type == 'MatMul' and add.op_type == 'Add', 'only ordered MatMul then Add is supported')
    require(all(not n.domain and not n.overload and not n.device_configurations and not n.attribute
                and len(n.input) == 2 and len(n.output) == 1 for n in graph.node),
            'custom domains, overloads, device configurations, attributes or nonbinary nodes are unsupported')
    x, y = graph.input[0], graph.output[0]
    m, k = static_shape(x)
    out_m, n = static_shape(y)
    require(m == out_m, 'batch shape mismatch')
    constants = {t.name: t for t in graph.initializer}
    require(len(constants) == 2 and all(constants), 'unique named constants required')
    require(mm.input[0] == x.name and mm.input[1] in constants, 'MatMul must consume input and constant weights')
    bias_names = set(constants) - {mm.input[1]}
    bias_name = next(iter(bias_names))
    require(list(add.input) in ([mm.output[0], bias_name], [bias_name, mm.output[0]])
            and add.output[0] == y.name, 'unsupported graph wiring')
    require(len({x.name, y.name, mm.output[0], *constants}) == 5
            and all((x.name, y.name, mm.output[0])), 'aliased or empty graph value names')
    for value in graph.value_info:
        require(value.name == mm.output[0] and static_shape(value) == (m, n), 'unsupported intermediate metadata')
    arrays = []
    for name, dims in ((mm.input[1], (k, n)), (bias_name, (n,))):
        t = constants[name]
        require(t.data_type == onnx.TensorProto.FLOAT and tuple(t.dims) == dims, 'constant FP32 shape mismatch')
        require(math.prod(dims) <= MAX_WEIGHTS, 'constant element limit exceeded')
        require(t.data_location == onnx.TensorProto.DEFAULT and not t.external_data and not t.HasField('segment'),
                'external or segmented constants are unsupported')
        require(not t.double_data and not t.int32_data and not t.int64_data and not t.uint64_data and not t.string_data,
                'mixed constant encodings are unsupported')
        require(bool(t.raw_data) != bool(t.float_data), 'exactly one FP32 constant encoding required')
        if t.raw_data:
            require(len(t.raw_data) == math.prod(dims) * 4, 'raw constant byte count mismatch')
        else:
            require(len(t.float_data) == math.prod(dims), 'constant element count mismatch')
        a = numpy_helper.to_array(t)
        require(np.isfinite(a).all(), 'nonfinite constants are unsupported')
        arrays.append(a.copy())
    # Validate ONNX schema/type/shape semantics as well as this stricter subset.
    onnx.checker.check_model(model, full_check=True)
    return (m, k, n), arrays


HEADER = '''#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Experimental artifact-local interface, not the frozen ShortHand runtime ABI.
 * FP32 native-endian contiguous buffers; counts are elements. Workspace must
 * be disjoint from input/output; input/output may overlap. Caller owns storage
 * and must prevent concurrent mutation. Use separate workspace per call.
 * Returns 0 on success, 1 on bad buffers, 2 on nonfinite input/result.
 * Output is committed only on success; workspace is unspecified on failure. */
int sh_aot_infer(const float *input, size_t input_count, float *output,
                 size_t output_count, float *workspace, size_t workspace_count);
#ifdef __cplusplus
}
#endif
'''


def source(shape, arrays):
    m, k, n = shape
    weights, bias = (','.join(float(v).hex() + 'f' for v in a.flat) for a in arrays)
    return f'''// Generated immutable model constants; no ONNX Runtime dependency.
#include "model.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559, "IEEE FP32 required");
namespace {{
constexpr float weights[] = {{{weights}}};
constexpr float bias[] = {{{bias}}};
bool region(const float *p, std::size_t count) {{
    const auto start=reinterpret_cast<std::uintptr_t>(p);
    return p && start % alignof(float)==0 && count <= (UINTPTR_MAX-start)/sizeof(float);
}}
bool overlap(const float *a,std::size_t an,const float *b,std::size_t bn) {{
    const auto x=reinterpret_cast<std::uintptr_t>(a),y=reinterpret_cast<std::uintptr_t>(b);
    return x < y+bn*sizeof(float) && y < x+an*sizeof(float);
}}
}}
extern "C" int sh_aot_infer(const float *input,std::size_t input_count,float *output,
                           std::size_t output_count,float *work,std::size_t work_count) {{
    if(input_count!={m*k} || output_count!={m*n} || work_count!={m*n} ||
       !region(input,input_count) || !region(output,output_count) || !region(work,work_count) ||
       overlap(work,work_count,input,input_count) || overlap(work,work_count,output,output_count)) return 1;
    for(std::size_t i=0;i<input_count;++i) if(!std::isfinite(input[i])) return 2;
    // k order is preserved. Vectorization across independent output columns
    // is legal without reassociation or relaxed floating-point semantics.
    for(std::size_t row=0;row<{m};++row) {{
        for(std::size_t col=0;col<{n};++col) work[row*{n}+col]=0.0f;
        for(std::size_t inner=0;inner<{k};++inner)
            for(std::size_t col=0;col<{n};++col)
                work[row*{n}+col] += input[row*{k}+inner]*weights[inner*{n}+col];
        for(std::size_t col=0;col<{n};++col) {{
            work[row*{n}+col] += bias[col];
            if(!std::isfinite(work[row*{n}+col])) return 2;
        }}
    }}
    std::memmove(output,work,output_count*sizeof(float));
    return 0;
}}
'''


def emit(model_path, output):
    # Read one bounded snapshot; record its digest rather than reopening it.
    with model_path.open('rb') as stream:
        data = stream.read(MAX_MODEL_BYTES + 1)
    shape, arrays = inspect_model(data)
    output.mkdir(parents=True, exist_ok=True)
    require(not any(output.iterdir()), 'use a fresh output directory')
    (output / 'model.h').write_text(HEADER)
    (output / 'model.cpp').write_text(source(shape, arrays))
    (output / 'model.onnx').write_bytes(data)
    manifest = dict(schema='shorthand.aot.static_affine.v1', experimental=True,
                    backend='C++17 source AOT; not MLIR/source-language integration',
                    shape=dict(batch=shape[0], inputs=shape[1], outputs=shape[2]),
                    model_sha256=hashlib.sha256(data).hexdigest(),
                    workspace_bytes=shape[0]*shape[2]*4,
                    precision='FP32; fast-math and contraction must be disabled',
                    source_sha256=hashlib.sha256((output/'model.cpp').read_bytes()).hexdigest(),
                    header_sha256=hashlib.sha256((output/'model.h').read_bytes()).hexdigest(),
                    latency_claim_authorized=False, energy_claim_authorized=False)
    (output/'manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True)+'\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('model', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    emit(args.model, args.output)


if __name__ == '__main__':
    main()
