#include "lib/library_order.h"
#include <string.h>

src_type_t src_type_detect(char const *filename) {
    char const *c = strrchr(filename, '.');
    if (!c) return src_type_others;
    switch (*++c) {
    case 'c':
        switch (*++c) {
        case '\0': return src_type_source;
        }
        return src_type_others;
    case 'h':
        switch (*++c) {
        case '\0': return src_type_header;
        }
        return src_type_others;
    }
    return src_type_others;
}
