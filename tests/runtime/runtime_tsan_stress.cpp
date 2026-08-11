#include "runtime/ShorthandRuntime.h"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

[[noreturn]] void fail(const std::string &message) {
    std::cerr << "error: " << message << '\n';
    std::exit(1);
}

void require(bool condition, const std::string &message) {
    if (!condition) fail(message);
}

bool nonempty(const char *value) {
    return value != nullptr && *value != '\0';
}

}  // namespace

int main() {
    constexpr int kWriters = 8;
    constexpr int kWriterIterations = 64;
    constexpr int kReaders = 6;
    constexpr int kReaderIterations = 500;

    require(short_runtime_reset() == SHORTHAND_RUNTIME_OK, "initial reset failed");

    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    threads.reserve(kWriters);

    for (int worker = 0; worker < kWriters; ++worker) {
        threads.emplace_back([worker, &failures] {
            for (int iteration = 0; iteration < kWriterIterations; ++iteration) {
                const std::string suffix = std::to_string(worker) + "_" + std::to_string(iteration);
                const std::string model = "tsan_model_" + suffix;
                const std::string input = "tsan_input_" + suffix;
                const std::string output = "tsan_output_" + suffix;
                if (short_ai_register_model(model.c_str(), "onnx", "missing.onnx", "inference",
                                            "float32", "1", "1", "fallback") != SHORTHAND_RUNTIME_OK)
                    ++failures;
                if (short_ai_register_tensor(input.c_str(), "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK)
                    ++failures;
                if (short_ai_register_tensor(output.c_str(), "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK)
                    ++failures;
                const int status = short_ai_infer(model.c_str(), input.c_str(), output.c_str());
                if (status != SHORTHAND_RUNTIME_NOT_EXECUTED && status != SHORTHAND_RUNTIME_BACKEND_UNAVAILABLE)
                    ++failures;
            }
        });
    }
    for (auto &thread : threads) thread.join();
    require(failures.load() == 0, "concurrent registration/inference returned unexpected status");

    std::atomic<bool> start{false};
    std::atomic<int> snapshot_failures{0};
    threads.clear();
    threads.reserve(kReaders + 2);

    for (int worker = 0; worker < kReaders; ++worker) {
        threads.emplace_back([&start, &snapshot_failures] {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            for (int iteration = 0; iteration < kReaderIterations; ++iteration) {
                if (!nonempty(short_runtime_observability_json()) ||
                    !nonempty(short_runtime_prometheus_metrics()) ||
                    !nonempty(short_runtime_otlp_spans_json()) ||
                    !nonempty(short_runtime_last_infer_telemetry_json())) {
                    ++snapshot_failures;
                    return;
                }
                (void)short_runtime_model_count();
                (void)short_runtime_tensor_count();
                (void)short_runtime_infer_count();
            }
        });
    }

    threads.emplace_back([&start] {
        while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
        for (int iteration = 0; iteration < 200; ++iteration)
            (void)short_ai_infer("", "", "");
    });

    threads.emplace_back([&start] {
        while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
        for (int iteration = 0; iteration < 64; ++iteration)
            (void)short_runtime_reset();
    });

    start.store(true, std::memory_order_release);
    for (auto &thread : threads) thread.join();
    require(snapshot_failures.load() == 0, "concurrent snapshots became invalid during reset/inference stress");

    require(short_runtime_reset() == SHORTHAND_RUNTIME_OK, "final reset failed");
    std::cout << "TSAN_STRESS writers=" << kWriters
              << " writer_iterations=" << kWriterIterations
              << " readers=" << kReaders
              << " reader_iterations=" << kReaderIterations << '\n';
    std::cout << "PASS ThreadSanitizer runtime race stress\n";
    return 0;
}
