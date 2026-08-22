#ifndef SHORTHAND_CORE_FFI_V1_H
#define SHORTHAND_CORE_FFI_V1_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) && defined(SHORTHAND_CORE_SHARED)
#if defined(SHORTHAND_CORE_BUILDING_LIBRARY)
#define SHORTHAND_CORE_API __declspec(dllexport)
#else
#define SHORTHAND_CORE_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) && defined(SHORTHAND_CORE_SHARED)
#define SHORTHAND_CORE_API __attribute__((visibility("default")))
#else
#define SHORTHAND_CORE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SHORTHAND_CORE_FFI_ABI_VERSION_MAJOR 1
#define SHORTHAND_CORE_FFI_ABI_VERSION_MINOR 0
#define SHORTHAND_CORE_FFI_ABI_VERSION_PATCH 0
#define SHORTHAND_CORE_FFI_ABI_VERSION "1.0.0"

typedef int32_t short_core_status;

enum short_core_status_code {
    SHORT_CORE_OK = 0,
    SHORT_CORE_INVALID_ARGUMENT = 1,
    SHORT_CORE_OUT_OF_RANGE = 2,
    SHORT_CORE_INVALID_UTF8 = 3,
    SHORT_CORE_ALLOCATION_FAILURE = 4,
    SHORT_CORE_WRONG_VARIANT = 5
};

#if defined(__cplusplus)
static_assert(sizeof(short_core_status) == 4, "short_core_status ABI width must be 32 bits");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(short_core_status) == 4, "short_core_status ABI width must be 32 bits");
#endif

typedef struct short_core_string short_core_string;

typedef struct short_core_string_view {
    const char *data;
    size_t length;
} short_core_string_view;

typedef struct short_core_slice_i32 {
    const int32_t *data;
    size_t length;
} short_core_slice_i32;

typedef struct short_core_option_f64 {
    uint8_t is_some;
    double value;
} short_core_option_f64;

typedef struct short_core_result_i32 {
    uint8_t is_ok;
    int32_t value;
    short_core_status error;
} short_core_result_i32;

SHORTHAND_CORE_API const char *short_core_abi_version(void);
SHORTHAND_CORE_API const char *short_core_status_name(short_core_status status);
SHORTHAND_CORE_API short_core_status short_core_string_create(
    const char *utf8, size_t length, short_core_string **out_string);
SHORTHAND_CORE_API short_core_status short_core_string_clone(
    const short_core_string *source, short_core_string **out_string);
SHORTHAND_CORE_API short_core_status short_core_string_view_get(
    const short_core_string *source, short_core_string_view *out_view);
SHORTHAND_CORE_API void short_core_string_destroy(short_core_string *value);
SHORTHAND_CORE_API short_core_status short_core_slice_i32_get(
    short_core_slice_i32 slice, size_t index, int32_t *out_value);
SHORTHAND_CORE_API short_core_option_f64 short_core_option_f64_none(void);
SHORTHAND_CORE_API short_core_option_f64 short_core_option_f64_some(double value);
SHORTHAND_CORE_API int short_core_option_f64_is_some(short_core_option_f64 option);
SHORTHAND_CORE_API short_core_status short_core_option_f64_value(
    short_core_option_f64 option, double *out_value);
SHORTHAND_CORE_API short_core_result_i32 short_core_result_i32_ok(int32_t value);
SHORTHAND_CORE_API short_core_result_i32 short_core_result_i32_error(short_core_status error);
SHORTHAND_CORE_API int short_core_result_i32_is_ok(short_core_result_i32 result);
SHORTHAND_CORE_API short_core_status short_core_result_i32_value(
    short_core_result_i32 result, int32_t *out_value);

#ifdef __cplusplus
}
#endif

#endif
