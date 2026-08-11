#include <runtime/ShorthandRuntime.h>

#include <cstring>
#include <iostream>

int main() {
    if (std::strcmp(short_runtime_abi_version(), "1.0.0") != 0) return 10;
    if (!short_runtime_is_abi_compatible(1, 0)) return 11;
    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 12;
    if (short_runtime_model_count() != 0) return 13;
    if (short_ai_register_tensor("input", "float", "1,4", "2", "4") != SHORTHAND_RUNTIME_OK) return 14;
    if (short_runtime_tensor_count() != 1) return 15;
    std::cout << "PASS installed runtime consumer\n";
    return 0;
}
