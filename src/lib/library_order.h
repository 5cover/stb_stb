#pragma once

#include "vec.h"

#include <stdbool.h>

VEC_DEFINE(str_vec_t, char const *);

typedef struct {
    char const *author, *brief, *copyright, *details, *version;
    char const *library;
    char const *output;
    str_vec_t dependencies; ///< user-included headers declared external with --dependency
    str_vec_t entries;      ///< public headers given with --entry; without any, every header is public
    str_vec_t source_files;
    bool overwrite, check;
} library_order_t;

typedef enum {
    src_type_others,
    src_type_source,
    src_type_header,
} src_type_t;

src_type_t src_type_detect(char const *filename);

/// @brief Packages the source files of @p order into a single STB-style header.
/// @return EXIT_SUCCESS or EXIT_FAILURE.
int library_order_deliver(library_order_t const *order);
