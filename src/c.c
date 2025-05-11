#include "lib/c.h"
#include <ctype.h>

/// @brief Read THEN input char (wint_t)
#define READ(C) *(C)++
/// @brief Run another rule on current input.
#define INCL(LABEL) goto LABEL
/// @brief Read next character, and run anothe rule.
#define THEN(LABEL, C) \
    do {               \
        C++;       \
        INCL(LABEL);   \
    } while (0)

slice_t extract_include(char const *const line) {
    slice_t filename = { .val = line };

    char const *c = line;

start:
    switch (READ(c)) {
    case '#':
        if (READ(c) == 'i'
            && READ(c) == 'n'
            && READ(c) == 'c'
            && READ(c) == 'l'
            && READ(c) == 'u'
            && READ(c) == 'd'
            && READ(c) == 'e') {
            THEN(start_path, c);
        }
        return (slice_t) { 0 };
    default:
        if (*c && isspace(*c)) goto start;
        return (slice_t) { 0 };
    }

start_path:
    switch (READ(c)) {
    case '<': THEN(usr, c);
    case '"': THEN(sys, c);
    default:
        if (*c && isspace(*c)) goto start_path;
        return (slice_t) { 0 };
    }

usr:
    switch (READ(c)) {
    case '"': return filename;
    case '\n':
    case '\0': return (slice_t) { 0 };
    default:
        filename.len++;
        goto usr;
    }

sys:
    switch (READ(c)) {
    case '>': return filename;
    case '\n':
    case '\0': return (slice_t) { 0 };
    default:
        filename.len++;
        goto sys;
    }
}
