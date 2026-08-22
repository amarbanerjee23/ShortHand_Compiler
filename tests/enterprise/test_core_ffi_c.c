#include "abi/shorthand_core_ffi_v1.h"

#include <stdint.h>
#include <string.h>

int main(void) {
    short_core_string *owned = NULL;
    short_core_string_view view = {0};
    const int32_t values[] = {3, 5, 8};
    int32_t selected = 0;
    double score = 0.0;

    if (strcmp(short_core_abi_version(), "1.0.0") != 0) return 10;
    if (short_core_string_create("sustainable", 11, &owned) != SHORT_CORE_OK) return 11;
    if (short_core_string_view_get(owned, &view) != SHORT_CORE_OK || view.length != 11) return 12;
    if (short_core_slice_i32_get((short_core_slice_i32){values, 3}, 2, &selected) != SHORT_CORE_OK || selected != 8) return 13;
    if (short_core_slice_i32_get((short_core_slice_i32){values, 3}, 3, &selected) != SHORT_CORE_OUT_OF_RANGE) return 14;
    if (short_core_option_f64_value(short_core_option_f64_some(2.5), &score) != SHORT_CORE_OK || score != 2.5) return 15;
    if (short_core_result_i32_value(short_core_result_i32_error(SHORT_CORE_INVALID_UTF8), &selected) != SHORT_CORE_INVALID_UTF8) return 16;

    short_core_string_destroy(owned);
    return 0;
}
