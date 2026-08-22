#ifndef SHORTHAND_CORE_HPP
#define SHORTHAND_CORE_HPP

#if __has_include(<shorthand/abi/shorthand_core_ffi_v1.h>)
#include <shorthand/abi/shorthand_core_ffi_v1.h>
#else
#include "../../../../abi/shorthand_core_ffi_v1.h"
#endif

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace shorthand::core::v1 {

class String final {
public:
    explicit String(std::string_view value) {
        const short_core_status status = short_core_string_create(value.data(), value.size(), &value_);
        if (status != SHORT_CORE_OK) throw std::runtime_error(short_core_status_name(status));
    }

    String(const String &other) {
        const short_core_status status = short_core_string_clone(other.value_, &value_);
        if (status != SHORT_CORE_OK) throw std::runtime_error(short_core_status_name(status));
    }

    String(String &&other) noexcept : value_(std::exchange(other.value_, nullptr)) {}

    String &operator=(String other) noexcept {
        swap(other);
        return *this;
    }

    ~String() { short_core_string_destroy(value_); }

    void swap(String &other) noexcept { std::swap(value_, other.value_); }

    std::string_view view() const {
        short_core_string_view result{};
        const short_core_status status = short_core_string_view_get(value_, &result);
        if (status != SHORT_CORE_OK) throw std::runtime_error(short_core_status_name(status));
        return std::string_view(result.data, result.length);
    }

private:
    short_core_string *value_ = nullptr;
};

}  // namespace shorthand::core::v1

#endif
