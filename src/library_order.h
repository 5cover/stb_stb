#pragma once

#include "input_field.h"
#include <stdbool.h>

typedef struct {
    input_field_t author, brief, copyright, details, version;
    char const *library;
    char const *output;
    char const **dependencies; ///< stbds array
    char const **source_files; ///< stbds array
    bool overwrite, check;
} library_order_t;

void library_order_describe(library_order_t const *p_order);