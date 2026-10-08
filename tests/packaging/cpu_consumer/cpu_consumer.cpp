#include <runtime/ShorthandRuntime.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>

int main(int argc, char **argv) {
    if (argc != 2 || short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 1;
    if (short_ai_register_tensor("input", "float32", "1", "1", "1") ||
        short_ai_register_tensor("output", "float32", "1", "1", "1") ||
        short_ai_register_model("identity", "onnx", argv[1], "identity", "float32", "1", "1", "onnxruntime_cpu")) return 2;
    for (float input : {42.0f, -7.25f, 0.0f, 1024.5f}) {
        float output = -999.0f;
        int count = -1;
        const int status = short_ai_infer_f32("identity", "input", &input, 1, "output", &output, 1, &count);
        if (status != SHORTHAND_RUNTIME_OK || count != 1 || output != input ||
            std::strcmp(short_runtime_last_infer_backend(), "onnxruntime_cpu") != 0) {
            std::cerr << "CPU execution failed: " << short_runtime_last_infer_reason() << '\n';
            return 3;
        }
    }
    if (std::strstr(short_runtime_last_infer_telemetry_json(), "\"route_cache_hit\":true") == nullptr) return 6;
    float bad = std::numeric_limits<float>::quiet_NaN(), unchanged = -999.0f;
    int count = -1;
    if (short_ai_infer_f32("identity", "input", &bad, 1, "output", &unchanged, 1, &count) == SHORTHAND_RUNTIME_OK ||
        unchanged != -999.0f) return 4;
    if (short_runtime_infer_success_count() != 4 || short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 5;
    std::cout << "PASS installed ONNX CPU: four exact outputs, warm reuse, finite rejection, reset\n";
}
