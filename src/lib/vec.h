#pragma once

#include <stdlib.h>

/// @brief Declares a growable array type @p name holding elements of type @p T.
#define VEC_DEFINE(name, T) \
    typedef struct {        \
        T *data;            \
        size_t len, cap;    \
    } name

/// @brief Appends @p x to the vec @p v.
#define vec_push(v, x)                                                    \
    do {                                                                  \
        if ((v)->len == (v)->cap) {                                       \
            (v)->cap = (v)->cap ? 2 * (v)->cap : 16;                      \
            (v)->data = realloc((v)->data, (v)->cap * sizeof *(v)->data); \
        }                                                                 \
        (v)->data[(v)->len++] = (x);                                      \
    } while (0)
