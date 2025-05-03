#include "library_order.h"
#include "lib/stb_ds.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

static inline int emit_doxygen_file(FILE *out, const char *filename) {
    FILE *in = fopen(filename, "r");
    if (!in) {
        fprintf(stderr, "error: cannot open %s: %s\n", filename, strerror(errno));
        return EX_IOERR;
    }

    char line[BUFSIZ];
    if (fgets(line, sizeof line, in)) {
        fputs(line, out);
    }
    while (fgets(line, sizeof line, in)) {
        fputs("///", out);
        if (line[0] != '\n') fputc(' ', out);
        fputs(line, out);
    }

    fclose(in);
    return EX_OK;
}

static inline void emit_doxygen_string(FILE *out, const char *s) {
    while (*s) {
        if (*s == '\n') {
            for (; *s == '\n'; s++) fputs("\n///", out);
            putc(' ', out);
        } else {
            putc(*s++, out);
        }
    }
}

static inline char *strtoupper(char const *s) {
    char *u = strdup(s);
    char *const r = u;
    while (*s) *u++ = toupper(*s++);
    return r;
}

static inline int put_doxygen(FILE *out, char const *command, input_field_t input) {
    switch (input.tag) {
    case input_field_null: break;
    case input_field_filename: {
        fprintf(out, "///%s ", command);
        int e = emit_doxygen_file(out, input.filename);
        if (e) return e;
        break;
    }
    case input_field_string: {
        fprintf(out, "///%s ", command);
        emit_doxygen_string(out, input.string);
        break;
    }
    }
    return EX_OK;
}

int library_order_deliver(library_order_t const *p_order, FILE *out) {
    fprintf(out, "/// @file\n");
    int e = put_doxygen(out, " @author", p_order->author);
    if (e) return e;
    e = put_doxygen(out, " @brief", p_order->brief);
    if (e) return e;
    e = put_doxygen(out, " @copyright", p_order->copyright);
    if (e) return e;
    e = put_doxygen(out, " @version", p_order->version);
    if (e) return e;

    fprintf(out, "/// @remark To build this, in one source file that includes this file do\n");
    fprintf(out, "/// @code\n");
    char *library_uc = strtoupper(p_order->library);
    fprintf(out, "/// #define STB_%s_IMPLEMENTATION\n", library_uc);
    fprintf(out, "/// @endcode\n");

    size_t n = arrlenu(p_order->dependencies);
    if (n) {
        fprintf(out, "/// @remark Dependencies:\n");
        for (size_t i = 0; i < n; ++i) printf("/// - %s\n", p_order->dependencies[i]);
    }

    e = put_doxygen(out, "\n///", p_order->details);

    free(library_uc);

    return e;
}

static void print_input_field(char const *name, input_field_t field) {
    printf("%s: ", name);
    switch (field.tag) {
    case input_field_null: puts("(none)"); break;
    case input_field_filename: printf("$(cat %s)\n", field.filename); break;
    case input_field_string: puts(field.string); break;
    }
}

void library_order_describe(library_order_t const *p_order) {
    printf("library: %s\n", p_order->library);

    print_input_field("author", p_order->author);
    print_input_field("brief", p_order->brief);
    print_input_field("copyright", p_order->copyright);
    print_input_field("details", p_order->details);
    print_input_field("version", p_order->version);

    printf("output: %s\n", p_order->output);

    size_t n = arrlenu(p_order->dependencies);
    printf("dependencies: %zu\n", n);
    for (size_t i = 0; i < n; ++i) {
        printf(" %zu. %s\n", i + 1, p_order->dependencies[i]);
    }

    n = arrlenu(p_order->source_files);
    printf("source files: %zu\n", n);
    for (size_t i = 0; i < n; ++i) {
        printf(" %zu. %s\n", i + 1, p_order->source_files[i]);
    }

    printf("flags:");
    if (p_order->overwrite) printf(" overwrite");
    if (p_order->check) printf(" check");
    putchar('\n');
}
