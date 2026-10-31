#include "lib/text.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool text_read(char const *path, char **text, size_t *len) {
    FILE *in = strcmp(path, "-") ? fopen(path, "r") : stdin;
    if (!in) {
        fprintf(stderr, "error: cannot open %s: %s\n", path, strerror(errno));
        return false;
    }

    size_t cap = 4096, n = 0;
    char *buf = malloc(cap);
    for (;;) {
        if (cap - n < 2) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        size_t got = fread(buf + n, 1, cap - n - 1, in);
        if (got == 0) break;
        n += got;
    }

    bool ok = !ferror(in);
    if (in != stdin) fclose(in);
    if (!ok) {
        fprintf(stderr, "error: cannot read %s: %s\n", path, strerror(errno));
        free(buf);
        return false;
    }

    buf[n] = '\0';
    *text = buf;
    *len = n;
    return true;
}
