#include <runtime/AIRuntimeBridgeAdapter.h>

#include <cstring>
#include <iostream>

int main() {
    using namespace shorthand;
    if (std::strcmp(runtime_bridge::bridgeAdapterContractVersion(),
                    "shorthand.runtime.ai_runtime_execution_adapter.v1") != 0) return 20;
    if (runtime_bridge::runtimeStatusFromInferenceStatus(ai::InferenceStatus::Success) !=
        SHORTHAND_RUNTIME_OK) return 21;
    if (runtime_bridge::runtimeStatusFromInferenceStatus(ai::InferenceStatus::NotExecuted) !=
        SHORTHAND_RUNTIME_NOT_EXECUTED) return 22;
    std::cout << "PASS installed AI bridge consumer\n";
    return 0;
}
