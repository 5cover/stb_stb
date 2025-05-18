#include "lib/c.h"
#include <ctype.h>

/// @brief Read THEN input char (wint_t)
#define READ(C) *(C)++
/// @brief Run another rule on current input.
#define INCL(LABEL) goto LABEL
/// @brief Read next character, and run anothe rule.
#define THEN(LABEL, C) \
    do {               \
        C++;           \
        INCL(LABEL);   \
    } while (0)

    // todo: should we THEN or INCL?

cnode_t parce_c(char const *const line) {
    cnode_t cnode;

    char const *c = line;

start:
    switch (READ(c)) {
    case '#':
        INCL(directive);
    default:
        if (*c && isspace(*c)) INCL(start);
        return (cnode_t) { 0 };
    }

directive:
    switch (READ(c)) {
    case 'i':
        if (READ(c) == 'n'
            && READ(c) == 'c'
            && READ(c) == 'l'
            && READ(c) == 'u'
            && READ(c) == 'd'
            && READ(c) == 'e') {
            cnode.tag = cnode_include;
            THEN(include, c);
        }
        return (cnode_t) { 0 };
    case 'p':
        if (READ(c) == 'r' && READ(c) == 'a' && READ(c) == 'g' && READ(c) == 'm' && READ(c) == 'a') {
            return (cnode_t) { 0 }; // THEN(pragma, c);
        }
        return (cnode_t) { 0 };
    default:
        return (cnode_t) { 0 };
    }

include:
    switch (READ(c)) {
    case '<':
        cnode.include.is_usr = true;
        cnode.include.file = (slice_t) { .len = 0, .val = c };
        THEN(include_sys, c);
    case '"':
        cnode.include.is_usr = false;
        cnode.include.file = (slice_t) { .len = 0, .val = c };
        THEN(include_usr, c);
    default:
        if (*c && isspace(*c)) INCL(include);
        return (cnode_t) { 0 };
    }

include_usr:
    switch (READ(c)) {
    case '"': return cnode;
    case '\n':
    case '\0': return (cnode_t) { 0 };
    default:
        cnode.include.file.len++;
        INCL(include_usr);
    }

include_sys:
    switch (READ(c)) {
    case '>': return cnode;
    case '\n':
    case '\0': return (cnode_t) { 0 };
    default:
        cnode.include.file.len++;
        INCL(include_sys);
    }
}
