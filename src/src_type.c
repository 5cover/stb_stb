#include "lib/library_order.h"

#include <string.h>

src_type_t src_type_detect(char const *filename) {
    char const *base = filename;
    for (char const *p = filename; *p; ++p) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }

    char const *ext = strrchr(base, '.');
    if (!ext) return src_type_others;
    if (!strcmp(ext, ".c")) return src_type_source;
    if (!strcmp(ext, ".h")) return src_type_header;
    return src_type_others;
}
