#include <abi/shorthand_runtime_abi_v1.h>

#include <iostream>

int main() {
    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 30;
    if (short_runtime_model_count() != 0) return 31;
    if (short_ai_register_tensor("legacy_input", "float", "1,2", "2", "2") != SHORTHAND_RUNTIME_OK) return 32;
    if (short_runtime_tensor_count() != 1) return 33;
    std::cout << "PASS frozen ABI v1 installed consumer\n";
    return 0;
}
