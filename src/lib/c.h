#pragma once

#include "vec.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    enum {
        c_directive_include,
        c_directive_include_computed, ///< #include followed by something other than a header name
        c_directive_pragma_once,
    } tag;
    bool is_usr;  ///< include only: "name" rather than <name>
    char *name;   ///< include only: the header name without delimiters
    size_t begin; ///< offset of the '#' in the original text
    size_t end;   ///< offset just past the directive's line, newline included
} c_directive_t;

VEC_DEFINE(c_directive_vec_t, c_directive_t);

/// @brief Appends the directives the packager cares about to @p out.
/// @remark Line splices, comments and string or character literals are handled as in translation phases 1 to 3.
/// @remark Macro expansion and conditional inclusion are not evaluated.
void c_scan(char const *text, size_t len, c_directive_vec_t *out);
