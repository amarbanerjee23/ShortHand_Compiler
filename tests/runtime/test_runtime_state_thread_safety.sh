#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
RUNTIME_LIB="${BUILD_DIR}/libshorthand_runtime.a"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

make -C "${SRC_DIR}" runtime_lib >/tmp/shorthand_runtime_state_make.out 2>&1 || {
  cat /tmp/shorthand_runtime_state_make.out >&2 || true
  exit 1
}

cat > "${WORK_DIR}/runtime_state_thread_safety.cpp" <<'CPP'
#include "runtime/ShorthandRuntime.h"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
[[noreturn]] void fail(int code, const std::string &message) {
    std::cerr << "error: " << message << "\n";
    std::exit(code);
}

void require(bool condition, int code, const std::string &message) {
    if (!condition) fail(code, message);
}

bool contains(const std::string &value, const std::string &needle) {
    return value.find(needle) != std::string::npos;
}
}

int main() {
    constexpr int kThreads = 8;
    require(short_runtime_reset() == SHORTHAND_RUNTIME_OK, 1, "initial reset failed");

    std::atomic<int> registration_failures{0};
    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        workers.emplace_back([i, &registration_failures] {
            const std::string suffix = std::to_string(i);
            const std::string model = "model_" + suffix;
            const std::string input = "input_" + suffix;
            const std::string output = "output_" + suffix;
            const std::string contract = "contract_" + suffix;
            const std::string workload = "workload_" + suffix;

            if (short_ai_register_model(model.c_str(), "onnx", "models/not_present.onnx",
                                        "inference", "float32", "1", "1", "fallback") != SHORTHAND_RUNTIME_OK) {
                ++registration_failures;
            }
            if (short_ai_register_tensor(input.c_str(), "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK) {
                ++registration_failures;
            }
            if (short_ai_register_tensor(output.c_str(), "float32", "1", "1", "1") != SHORTHAND_RUNTIME_OK) {
                ++registration_failures;
            }
            if (short_greenai_register_contract(contract.c_str(), "inference", "completed",
                                                "runtime", "measured", "declared",
                                                "location_based", "candidate_only") != SHORTHAND_RUNTIME_OK) {
                ++registration_failures;
            }
            if (short_greenai_record_measurement(workload.c_str(), "fallback", "1", "10", "1") != SHORTHAND_RUNTIME_OK) {
                ++registration_failures;
            }
        });
    }
    for (auto &worker : workers) worker.join();

    require(registration_failures.load() == 0, 2, "concurrent registration returned failures");
    require(short_runtime_model_count() == kThreads, 3, "model count is not linearizable");
    require(short_runtime_tensor_count() == kThreads * 2, 4, "tensor count is not linearizable");
    require(short_runtime_contract_count() == kThreads, 5, "contract count is not linearizable");
    require(short_runtime_measurement_count() == kThreads, 6, "measurement count is not linearizable");

    std::atomic<int> inference_failures{0};
    workers.clear();
    for (int i = 0; i < kThreads; ++i) {
        workers.emplace_back([i, &inference_failures] {
            const std::string suffix = std::to_string(i);
            const std::string model = "model_" + suffix;
            const std::string input = "input_" + suffix;
            const std::string output = "output_" + suffix;
            const int status = short_ai_infer(model.c_str(), input.c_str(), output.c_str());
            if (status != SHORTHAND_RUNTIME_NOT_EXECUTED) ++inference_failures;
        });
    }
    for (auto &worker : workers) worker.join();

    require(inference_failures.load() == 0, 7, "concurrent fallback inference returned an unexpected status");
    require(short_runtime_infer_count() == kThreads, 8, "inference count mismatch");
    require(short_runtime_infer_not_executed_count() == kThreads, 9, "not-executed count mismatch");
    require(short_runtime_infer_success_count() == 0, 10, "fallback path reported success");

    std::atomic<int> snapshot_failures{0};
    workers.clear();
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&snapshot_failures] {
            for (int iteration = 0; iteration < 100; ++iteration) {
                const std::string observability = short_runtime_observability_json();
                const std::string prometheus = short_runtime_prometheus_metrics();
                const std::string otlp = short_runtime_otlp_spans_json();
                const std::string telemetry = short_runtime_last_infer_telemetry_json();
                if (!contains(observability, "shorthand.runtime.observability.v1") ||
                    !contains(prometheus, "shorthand_runtime_infer_total") ||
                    !contains(otlp, "shorthand.runtime.otlp_spans.v1") ||
                    !contains(telemetry, "shorthand.runtime.infer_telemetry.v1")) {
                    ++snapshot_failures;
                    return;
                }
            }
        });
    }
    for (auto &worker : workers) worker.join();
    require(snapshot_failures.load() == 0, 11, "concurrent evidence snapshot was malformed");

    std::atomic<bool> captured{false};
    std::atomic<bool> changed{false};
    std::atomic<int> pointer_failures{0};
    std::thread snapshot_owner([&] {
        const char *reason_pointer = short_runtime_last_infer_reason();
        const std::string expected = reason_pointer ? reason_pointer : "";
        captured.store(true, std::memory_order_release);
        while (!changed.load(std::memory_order_acquire)) std::this_thread::yield();
        if (reason_pointer == nullptr || std::string(reason_pointer) != expected) ++pointer_failures;
    });
    std::thread state_changer([&] {
        while (!captured.load(std::memory_order_acquire)) std::this_thread::yield();
        (void)short_ai_infer("", "", "");
        changed.store(true, std::memory_order_release);
    });
    snapshot_owner.join();
    state_changer.join();
    require(pointer_failures.load() == 0, 12, "another thread invalidated a string snapshot");

    std::atomic<int> reset_reader_failures{0};
    workers.clear();
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&reset_reader_failures] {
            for (int iteration = 0; iteration < 100; ++iteration) {
                const char *json = short_runtime_observability_json();
                if (json == nullptr || !contains(json, "shorthand.runtime.observability.v1")) {
                    ++reset_reader_failures;
                    return;
                }
            }
        });
    }
    workers.emplace_back([] { (void)short_runtime_reset(); });
    for (auto &worker : workers) worker.join();
    require(reset_reader_failures.load() == 0, 13, "reset raced with an invalid observability snapshot");

    require(short_runtime_reset() == SHORTHAND_RUNTIME_OK, 14, "final reset failed");
    require(short_runtime_model_count() == 0, 15, "models remained after reset");
    require(short_runtime_tensor_count() == 0, 16, "tensors remained after reset");
    require(short_runtime_contract_count() == 0, 17, "contracts remained after reset");
    require(short_runtime_measurement_count() == 0, 18, "measurements remained after reset");
    require(short_runtime_infer_count() == 0, 19, "inference counters remained after reset");

    std::cout << "RUNTIME_STATE context=process_wide synchronization=serialized_public_abi\n";
    std::cout << "RUNTIME_STATE string_snapshots=thread_local tenant_isolation=process_boundary\n";
    std::cout << "PASS runtime state isolation and thread-safety gate\n";
    return 0;
}
CPP

${CXX:-g++} -std=c++17 -Wall -Wextra -Wpedantic -pthread \
  -I"${SRC_DIR}" \
  "${WORK_DIR}/runtime_state_thread_safety.cpp" \
  "${RUNTIME_LIB}" \
  -o "${WORK_DIR}/runtime_state_thread_safety"

"${WORK_DIR}/runtime_state_thread_safety" \
  >/tmp/shorthand_runtime_state_thread_safety.out \
  2>/tmp/shorthand_runtime_state_thread_safety.err

grep -q 'PASS runtime state isolation and thread-safety gate' /tmp/shorthand_runtime_state_thread_safety.out
grep -q 'context=process_wide synchronization=serialized_public_abi' /tmp/shorthand_runtime_state_thread_safety.out
grep -q 'string_snapshots=thread_local tenant_isolation=process_boundary' /tmp/shorthand_runtime_state_thread_safety.out

public_symbol_count="$(nm -g --defined-only "${RUNTIME_LIB}" | awk '{print $NF}' | grep '^short_' | sort -u | wc -l | tr -d ' ')"
if [[ "${public_symbol_count}" != "25" ]]; then
  echo "error: synchronized runtime changed the frozen ABI v1 symbol count: ${public_symbol_count}" >&2
  nm -g --defined-only "${RUNTIME_LIB}" >&2 || true
  exit 1
fi

cat /tmp/shorthand_runtime_state_thread_safety.out
