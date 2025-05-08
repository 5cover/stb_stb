#pragma once

#include "slice.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    enum {
        c_ast_node_include,
    } tag;
    union {
        struct {
            enum {
                c_ast_node_include_scope_user,
                c_ast_node_include_scope_system,
            } scope;
            slice_t path;
        } include;
    } node;
} c_ast_node;

void parse_c(FILE *);
