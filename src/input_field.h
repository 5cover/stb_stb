#pragma once

typedef struct {
    enum {
        input_field_null,
        input_field_string,
        input_field_pathname,
    } tag;
    union {
        char const *string;
        char const *pathname;
    };
} input_field_t;

#define input_field_pathname(value) (input_field_t) { .tag = input_field_pathname, .pathname = (value) };
#define input_field_string(value) (input_field_t) { .tag = input_field_string, .string = (value) };
