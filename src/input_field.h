#pragma once

typedef struct {
    enum {
        input_field_null = 0,
        input_field_string,
        input_field_filename,
    } tag;
    union {
        char const *string;
        char const *filename;
    };
} input_field_t;

#define input_field_filename(value) (input_field_t) { .tag = input_field_filename, .filename = (value) };
#define input_field_string(value) (input_field_t) { .tag = input_field_string, .string = (value) };
