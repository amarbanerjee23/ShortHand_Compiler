#include "abi/shorthand_core_ffi_v1.h"
#include "Compiler_new_ws/Short_Hand/src/core/ShorthandCore.hpp"

#include <cstring>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, const char *message) {
    if (condition) return;
    std::cerr << "FAIL " << message << '\n';
    ++failures;
}

}  // namespace

int main() {
    expect(sizeof(short_core_status) == sizeof(std::int32_t), "fixed-width core status ABI");
    expect(std::strcmp(short_core_abi_version(), "1.0.0") == 0, "versioned core ABI");
    short_core_string *value = nullptr;
    expect(short_core_string_create("valid", 5, &value) == SHORT_CORE_OK, "owned UTF-8 string");
    short_core_string *clone = nullptr;
    expect(short_core_string_clone(value, &clone) == SHORT_CORE_OK, "explicit clone");
    short_core_string *failed_clone = value;
    expect(short_core_string_clone(nullptr, &failed_clone) == SHORT_CORE_INVALID_ARGUMENT && failed_clone == nullptr,
           "failed clone clears output ownership handle");
    short_core_string_view view{};
    expect(short_core_string_view_get(clone, &view) == SHORT_CORE_OK && view.length == 5,
           "borrowed string view");
    expect(short_core_string_view_get(nullptr, &view) == SHORT_CORE_INVALID_ARGUMENT &&
               view.data == nullptr && view.length == 0U,
           "failed borrowed view clears output");
    const char invalid[] = {static_cast<char>(0xc0), static_cast<char>(0x80)};
    short_core_string *rejected = value;
    expect(short_core_string_create(invalid, sizeof(invalid), &rejected) == SHORT_CORE_INVALID_UTF8 && rejected == nullptr,
           "overlong UTF-8 rejected without leaking an output handle");
    short_core_string_destroy(value);
    short_core_string_destroy(clone);

    const std::int32_t storage[] = {4, 9};
    std::int32_t selected = 0;
    expect(short_core_slice_i32_get({storage, 2}, 1, &selected) == SHORT_CORE_OK && selected == 9,
           "bounded slice access");
    expect(short_core_slice_i32_get({storage, 2}, 2, &selected) == SHORT_CORE_OUT_OF_RANGE,
           "slice boundary rejection");
    double score = 0.0;
    expect(short_core_option_f64_value(short_core_option_f64_none(), &score) == SHORT_CORE_WRONG_VARIANT,
           "option wrong variant rejection");
    expect(short_core_result_i32_value(short_core_result_i32_ok(42), &selected) == SHORT_CORE_OK && selected == 42,
           "result ok value");

    shorthand::core::v1::String cpp_value("namespace-safe");
    expect(cpp_value.view() == std::string_view("namespace-safe"), "C++ namespace and RAII wrapper");
    if (failures != 0) return 1;
    std::cout << "PASS safe C and C++ core FFI\n";
    return 0;
}
