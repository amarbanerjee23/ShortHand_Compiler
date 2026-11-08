#pragma once
#include <stdint.h>

// Experimental, artifact-local v1 support. Not part of the frozen runtime ABI.
// Callers own live, suitably aligned FP32 arrays for the duration of the call.
// Workspace must be disjoint from both arrays. Input/output may overlap.
#ifdef __cplusplus
extern "C" {
#endif
// 0 success, 1 invalid region/count, 2 non-finite data, 3 CPU policy denied,
// 4 unsupported floating-point environment. Failure never changes output.
int32_t sh_aot_v1_prepare(const float *input, uint64_t input_count,
                        float *output, uint64_t output_count,
                        float *workspace, uint64_t workspace_count,
                        uint64_t expected_input, uint64_t expected_output);
int32_t sh_aot_v1_commit(float *output, uint64_t output_count,
                       const float *workspace, uint64_t workspace_count);
#ifdef __cplusplus
}
#endif
