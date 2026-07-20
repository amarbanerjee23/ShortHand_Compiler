#pragma once

#include "ShorthandRuntime.h"
#include "ai_runtime/AI_Types.h"

namespace shorthand::runtime_bridge {

struct RuntimeBridgeModelInput {
    const char *name;
    const char *format;
    const char *path;
    const char *task;
    const char *precision;
    const char *input_shape;
    const char *output_shape;
    const char *backend_preference;
    bool allow_fallback;
};

struct RuntimeBridgeTensorInput {
    const char *name;
    const char *element_type;
    const char *shape;
    const char *rank;
    const char *total_elements;
};

const char *bridgeAdapterContractVersion();

ai::ModelSpec buildModelSpec(const RuntimeBridgeModelInput &model,
                             const RuntimeBridgeTensorInput &input,
                             const RuntimeBridgeTensorInput &output);

ai::TensorBuffer buildInputTensorBuffer(const RuntimeBridgeTensorInput &input,
                                        const float *input_values,
                                        int input_count);

int runtimeStatusFromInferenceStatus(ai::InferenceStatus status);

bool bridgeRequestIsExecutionReady(const ai::ModelSpec &model,
                                   const ai::TensorBuffer &input,
                                   int output_capacity);

} // namespace shorthand::runtime_bridge
