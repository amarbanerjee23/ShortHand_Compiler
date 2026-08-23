#include <shorthand/core/ShorthandCore.hpp>

#include <string_view>
#include <utility>

int main() {
    shorthand::core::v1::String original("enterprise");
    shorthand::core::v1::String copy(original);
    shorthand::core::v1::String moved(std::move(copy));
    return original.view() == std::string_view("enterprise") &&
                   moved.view() == std::string_view("enterprise")
               ? 0
               : 1;
}
