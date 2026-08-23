#include "abi/shorthand_core_ffi_v1.h"

#include <new>
#include <cstdint>
#include <string>

struct short_core_string {
    std::string bytes;
};

namespace {

bool validUtf8(const char *data, std::size_t length) {
    std::size_t index = 0;
    while (index < length) {
        const unsigned char first = static_cast<unsigned char>(data[index]);
        if (first <= 0x7fU) {
            ++index;
            continue;
        }
        std::size_t continuation = 0;
        std::uint32_t value = 0;
        if ((first & 0xe0U) == 0xc0U) {
            continuation = 1;
            value = first & 0x1fU;
            if (value < 2U) return false;
        } else if ((first & 0xf0U) == 0xe0U) {
            continuation = 2;
            value = first & 0x0fU;
        } else if ((first & 0xf8U) == 0xf0U) {
            continuation = 3;
            value = first & 0x07U;
        } else {
            return false;
        }
        if (continuation > length - index - 1U) return false;
        for (std::size_t i = 0; i < continuation; ++i) {
            const unsigned char next = static_cast<unsigned char>(data[index + i + 1U]);
            if ((next & 0xc0U) != 0x80U) return false;
            value = (value << 6U) | (next & 0x3fU);
        }
        if ((continuation == 2U && value < 0x800U) ||
            (continuation == 3U && value < 0x10000U) ||
            value > 0x10ffffU || (value >= 0xd800U && value <= 0xdfffU)) return false;
        index += continuation + 1U;
    }
    return true;
}

}  // namespace

extern "C" {

const char *short_core_abi_version(void) {
    return SHORTHAND_CORE_FFI_ABI_VERSION;
}

const char *short_core_status_name(short_core_status status) {
    switch (status) {
        case SHORT_CORE_OK: return "ok";
        case SHORT_CORE_INVALID_ARGUMENT: return "invalid_argument";
        case SHORT_CORE_OUT_OF_RANGE: return "out_of_range";
        case SHORT_CORE_INVALID_UTF8: return "invalid_utf8";
        case SHORT_CORE_ALLOCATION_FAILURE: return "allocation_failure";
        case SHORT_CORE_WRONG_VARIANT: return "wrong_variant";
    }
    return "unknown";
}

short_core_status short_core_string_create(const char *utf8,
                                           size_t length,
                                           short_core_string **out_string) {
    if (out_string == nullptr || (utf8 == nullptr && length != 0U)) return SHORT_CORE_INVALID_ARGUMENT;
    *out_string = nullptr;
    const char *bytes = utf8 == nullptr ? "" : utf8;
    if (!validUtf8(bytes, length)) return SHORT_CORE_INVALID_UTF8;
    short_core_string *value = new (std::nothrow) short_core_string;
    if (value == nullptr) return SHORT_CORE_ALLOCATION_FAILURE;
    try {
        value->bytes.assign(bytes, length);
    } catch (...) {
        delete value;
        return SHORT_CORE_ALLOCATION_FAILURE;
    }
    *out_string = value;
    return SHORT_CORE_OK;
}

short_core_status short_core_string_clone(const short_core_string *source,
                                          short_core_string **out_string) {
    if (out_string != nullptr) *out_string = nullptr;
    if (source == nullptr || out_string == nullptr) return SHORT_CORE_INVALID_ARGUMENT;
    return short_core_string_create(source->bytes.data(), source->bytes.size(), out_string);
}

short_core_status short_core_string_view_get(const short_core_string *source,
                                             short_core_string_view *out_view) {
    if (out_view != nullptr) {
        out_view->data = nullptr;
        out_view->length = 0U;
    }
    if (source == nullptr || out_view == nullptr) return SHORT_CORE_INVALID_ARGUMENT;
    out_view->data = source->bytes.data();
    out_view->length = source->bytes.size();
    return SHORT_CORE_OK;
}

void short_core_string_destroy(short_core_string *value) {
    delete value;
}

short_core_status short_core_slice_i32_get(short_core_slice_i32 slice,
                                           size_t index,
                                           int32_t *out_value) {
    if (out_value == nullptr || (slice.data == nullptr && slice.length != 0U)) return SHORT_CORE_INVALID_ARGUMENT;
    if (index >= slice.length) return SHORT_CORE_OUT_OF_RANGE;
    *out_value = slice.data[index];
    return SHORT_CORE_OK;
}

short_core_option_f64 short_core_option_f64_none(void) {
    return short_core_option_f64{0U, 0.0};
}

short_core_option_f64 short_core_option_f64_some(double value) {
    return short_core_option_f64{1U, value};
}

int short_core_option_f64_is_some(short_core_option_f64 option) {
    return option.is_some == 1U ? 1 : 0;
}

short_core_status short_core_option_f64_value(short_core_option_f64 option,
                                              double *out_value) {
    if (out_value == nullptr) return SHORT_CORE_INVALID_ARGUMENT;
    if (option.is_some != 1U) return SHORT_CORE_WRONG_VARIANT;
    *out_value = option.value;
    return SHORT_CORE_OK;
}

short_core_result_i32 short_core_result_i32_ok(int32_t value) {
    return short_core_result_i32{1U, value, SHORT_CORE_OK};
}

short_core_result_i32 short_core_result_i32_error(short_core_status error) {
    if (error == SHORT_CORE_OK) error = SHORT_CORE_INVALID_ARGUMENT;
    return short_core_result_i32{0U, 0, error};
}

int short_core_result_i32_is_ok(short_core_result_i32 result) {
    return result.is_ok == 1U ? 1 : 0;
}

short_core_status short_core_result_i32_value(short_core_result_i32 result,
                                              int32_t *out_value) {
    if (out_value == nullptr) return SHORT_CORE_INVALID_ARGUMENT;
    if (result.is_ok != 1U) return result.error == SHORT_CORE_OK ? SHORT_CORE_WRONG_VARIANT : result.error;
    *out_value = result.value;
    return SHORT_CORE_OK;
}

}  // extern "C"
