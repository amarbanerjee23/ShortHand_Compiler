#include "runtime/ShorthandRuntime.h"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
[[noreturn]] void fail(const char *message) {
    std::cerr << "error: " << message << '\n';
    std::exit(2);
}

bool contains(const char *value, const char *needle) {
    return value != nullptr && std::string(value).find(needle) != std::string::npos;
}
}  // namespace

int main() {
    constexpr int kWorkers = 8;
    constexpr int kIterations = 150;
    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) fail("initial reset failed");

    std::atomic<int> api_failures{0};
    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int worker = 0; worker < kWorkers; ++worker) {
        workers.emplace_back([worker, &api_failures] {
            const std::string suffix = std::to_string(worker);
            const std::string model = "stress_model_" + suffix;
            const std::string input = "stress_input_" + suffix;
            const std::string output = "stress_output_" + suffix;
            const std::string contract = "stress_contract_" + suffix;
            const std::string workload = "stress_workload_" + suffix;

            if (short_ai_register_model(model.c_str(), "onnx", "missing.onnx", "inference",
                                        "float32", "1", "1", "fallback") != SHORTHAND_RUNTIME_OK)
                ++api_failures;
            if (short_ai_register_tensor(input.c_str(), "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK)
                ++api_failures;
            if (short_ai_register_tensor(output.c_str(), "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK)
                ++api_failures;
            if (short_greenai_register_contract(contract.c_str(), "inference", "completed", "runtime",
                                                "measured", "declared", "location_based",
                                                "candidate_only") != SHORTHAND_RUNTIME_OK)
                ++api_failures;
            if (short_greenai_record_measurement(workload.c_str(), "fallback", "1", "10", "1") !=
                SHORTHAND_RUNTIME_OK)
                ++api_failures;

            for (int iteration = 0; iteration < kIterations; ++iteration) {
                (void)short_runtime_model_count();
                (void)short_runtime_tensor_count();
                (void)short_runtime_contract_count();
                (void)short_ai_infer(model.c_str(), input.c_str(), output.c_str());
                if (!contains(short_runtime_observability_json(), "shorthand.runtime.observability.v1"))
                    ++api_failures;
                if (!contains(short_runtime_prometheus_metrics(), "shorthand_runtime_infer_total"))
                    ++api_failures;
                if (!contains(short_runtime_otlp_spans_json(), "shorthand.runtime.otlp_spans.v1"))
                    ++api_failures;
            }
        });
    }

    std::atomic<bool> snapshot_ready{false};
    std::atomic<bool> state_changed{false};
    std::thread snapshot_owner([&] {
        const char *pointer = short_runtime_last_infer_reason();
        const std::string expected = pointer == nullptr ? std::string() : std::string(pointer);
        snapshot_ready.store(true, std::memory_order_release);
        while (!state_changed.load(std::memory_order_acquire)) std::this_thread::yield();
        if (pointer == nullptr || std::string(pointer) != expected) ++api_failures;
    });
    std::thread state_changer([&] {
        while (!snapshot_ready.load(std::memory_order_acquire)) std::this_thread::yield();
        (void)short_ai_infer("", "", "");
        state_changed.store(true, std::memory_order_release);
    });

    for (auto &worker : workers) worker.join();
    snapshot_owner.join();
    state_changer.join();

    if (api_failures.load() != 0) fail("runtime API consistency failed under concurrent stress");

    std::atomic<int> snapshot_failures{0};
    workers.clear();
    for (int worker = 0; worker < 6; ++worker) {
        workers.emplace_back([&snapshot_failures] {
            for (int iteration = 0; iteration < kIterations; ++iteration) {
                if (!contains(short_runtime_observability_json(), "shorthand.runtime.observability.v1"))
                    ++snapshot_failures;
                (void)short_runtime_last_infer_backend();
                (void)short_runtime_last_infer_telemetry_json();
            }
        });
    }
    workers.emplace_back([] {
        for (int iteration = 0; iteration < 50; ++iteration) (void)short_runtime_reset();
    });
    for (auto &worker : workers) worker.join();

    if (snapshot_failures.load() != 0) fail("snapshot malformed during concurrent reset");
    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) fail("final reset failed");

    std::cout << "RUNTIME_STRESS workers=" << kWorkers << " iterations=" << kIterations
              << " facade=serialized_public_abi\n";
    std::cout << "PASS runtime concurrency stress\n";
    return 0;
}
