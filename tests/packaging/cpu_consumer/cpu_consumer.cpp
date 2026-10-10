#include <runtime/ShorthandRuntime.h>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <filesystem>
#endif

int main(int argc, char **argv) {
#ifdef _WIN32
    wchar_t executable[32768] = {}, runtime[32768] = {};
    const HMODULE module = GetModuleHandleW(L"onnxruntime.dll");
    const DWORD exeLength = GetModuleFileNameW(nullptr, executable, 32768);
    const DWORD dllLength = module ? GetModuleFileNameW(module, runtime, 32768) : 0;
    std::error_code error;
    if (!exeLength || exeLength >= 32768 || !dllLength || dllLength >= 32768 ||
        !std::filesystem::equivalent(std::filesystem::path(executable).parent_path(),
                                     std::filesystem::path(runtime).parent_path(), error) || error) {
        std::cerr << "CPU consumer must load its app-local ONNX Runtime DLL\n";
        return 10;
    }
    std::cout << "PASS loaded app-local ONNX Runtime DLL\n";
#endif
    if ((argc != 2 && argc != 3) || short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 1;
    if (short_ai_register_tensor("input", "float32", "1", "1", "1") ||
        short_ai_register_tensor("output", "float32", "1", "1", "1") ||
        short_ai_register_model("identity", "onnx", argv[1], "identity", "float32", "1", "1", "onnxruntime_cpu")) return 2;
    if (argc == 3) {
        if (std::strcmp(argv[2], "--expect-unqualified") != 0) return 7;
        float input = 42.0f, output = -999.0f;
        int count = -1;
        const int status = short_ai_infer_f32("identity", "input", &input, 1, "output", &output, 1, &count);
        if (status == SHORTHAND_RUNTIME_OK || output != -999.0f || count != 0 ||
            short_runtime_infer_success_count() != 0 ||
            std::strcmp(short_runtime_last_infer_reason(), "backend_device_not_production_qualified") != 0 ||
            std::strstr(short_runtime_last_infer_telemetry_json(), "\"experimental_override\":false") == nullptr) return 8;
        std::cout << "PASS existing production scope rejects unqualified native execution by default\n";
        return 0;
    }
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
    const bool experimental = std::getenv("SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE") != nullptr;
    if (std::strstr(short_runtime_last_infer_telemetry_json(), experimental ?
                    "\"experimental_override\":true" : "\"experimental_override\":false") == nullptr ||
        std::strstr(short_runtime_last_infer_telemetry_json(), experimental ?
                    "\"production_qualified\":false" : "\"production_qualified\":true") == nullptr) return 9;
    float bad = std::numeric_limits<float>::quiet_NaN(), unchanged = -999.0f;
    int count = -1;
    if (short_ai_infer_f32("identity", "input", &bad, 1, "output", &unchanged, 1, &count) == SHORTHAND_RUNTIME_OK ||
        unchanged != -999.0f) return 4;
    if (short_runtime_infer_success_count() != 4 || short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 5;
    std::cout << "PASS installed ONNX CPU: four exact outputs, warm reuse, finite rejection, reset\n";
}
