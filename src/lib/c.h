#pragma once

#include "slice.h"
#include <stdbool.h>

typedef struct {
    enum {
        cnode_unknown,
        cnode_include,
        cnode_pragma_once
    } tag;
    union {
        struct {
            bool is_usr;
            slice_t file;
        } include;
    };
} cnode_t;

cnode_t parce_c(char const *line);