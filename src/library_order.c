#include "library_order.h"
#include "lib/stb_ds.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

typedef struct {
    enum {
        emit_doxygen,
        emit_string_constant,
    } tag;
    union {
        struct {
            char const *command;
        } doxygen;
    };
} emit_mode_t;

#define fputln(stream, str) fputs(str "\n", out)
#define fputlnf(stream, fmt, ...) fprintf(stream, fmt "\n", __VA_ARGS__)

static inline char *fgets_until_next_line(char *restrict s, int n, FILE *stream) {
    while ((s = fgets(s, n, stream)) && s[strlen(s) - 1] != '\n');
    return s;
}

static inline int embed_file(FILE *out, const char *filename) {
    FILE *in = fopen(filename, "r");
    if (!in) {
        fputlnf(stderr, "error: cannot open %s: %s", filename, strerror(errno));
        return EX_IOERR;
    }

    char line[BUFSIZ];
    while (fgets(line, sizeof line, in)) {
        // todo: remove includes if file is in source files and not in dependencies
        fputs(line, out);
    }

    fclose(in);

    putc('\n', out);

    return EX_OK;
}

static inline int emit_file(emit_mode_t const emit, FILE *out, const char *filename) {
    FILE *in = fopen(filename, "r");
    if (!in) {
        fputlnf(stderr, "error: cannot open %s: %s", filename, strerror(errno));
        return EX_IOERR;
    }

    char line[BUFSIZ];
    if (fgets_until_next_line(line, sizeof line, in)) fputs(line, out);
    while (fgets_until_next_line(line, sizeof line, in)) {
        switch (emit.tag) {
        case emit_string_constant:
            fputs("\\n\\", out);
            break;
        case emit_doxygen:
            fputs("///", out);
            if (line[0] != '\n') putc(' ', out);
            break;
        }
        fputs(line, out);
    };

    fclose(in);
    return EX_OK;
}

static inline void emit_string(emit_mode_t const emit, FILE *out, const char *s) {
    while (*s) {
        switch (emit.tag) {
        case emit_string_constant:
            if (*s == '\n') fputs("\\n\\", out);
            putc(*s++, out);
            break;
        case emit_doxygen:
            if (*s == '\n') {
                for (; *s == '\n'; s++) fputs("\n///", out);
                putc(' ', out);
            } else {
                putc(*s++, out);
            }
            break;
        default:;
        }
    }
}

static inline int put_field(FILE *out, emit_mode_t const emit, input_field_t input) {
    if (input.tag == input_field_null) return EX_OK;
    switch (emit.tag) {
    case emit_doxygen:
        fprintf(out, "///%s ", emit.doxygen.command);
        break;
    case emit_string_constant:
        putc('"', out);
        break;
    }
    switch (input.tag) {
    case input_field_filename: {
        int e = emit_file(emit, out, input.filename);
        if (e) return e;
        break;
    }
    case input_field_string: {
        emit_string(emit, out, input.string);
        break;
    }
    default:;
    }
    switch (emit.tag) {
    case emit_doxygen:
        putc('\n', out);
        break;
    case emit_string_constant:
        putc('"', out);
        break;
    }
    return EX_OK;
}

static inline int put_field_doxygen(FILE *out, char const *command, input_field_t input) {
    return put_field(out, (emit_mode_t) { .tag = emit_doxygen, .doxygen.command = command }, input);
}

static inline int put_field_string_constant(FILE *out, input_field_t input) {
    return put_field(out, (emit_mode_t) { .tag = emit_string_constant }, input);
}

static inline char *strtoupper(char const *s) {
    char *u = strdup(s);
    char *const r = u;
    while (*s) *u++ = toupper(*s++);
    return r;
}

int library_order_deliver(library_order_t const *p_order, FILE *out) {
    fputln(out, "/// @file");
    int e = put_field_doxygen(out, " @author", p_order->author);
    if (e) return e;
    e = put_field_doxygen(out, " @brief", p_order->brief);
    if (e) return e;
    e = put_field_doxygen(out, " @copyright", p_order->copyright);
    if (e) return e;
    e = put_field_doxygen(out, " @version", p_order->version);
    if (e) return e;

    fputln(out, "/// @remark To build this, in one source file that includes this file do");
    fputln(out, "/// @code");
    char *library_uc = strtoupper(p_order->library);
#define FREE_RETURN(value) \
    do {                   \
        free(library_uc);  \
        return value;      \
    } while (0)
    fputlnf(out, "/// #define STB_%s_IMPLEMENTATION", library_uc);
    fputln(out, "/// @endcode");

    size_t n = arrlenu(p_order->dependencies);
    if (n) {
        fputln(out, "/// @remark Dependencies:");
        for (size_t i = 0; i < n; ++i) fputlnf(out, "/// - %s", p_order->dependencies[i]);
    }

    e = put_field_doxygen(out, "\n///", p_order->details);
    if (e) FREE_RETURN(e);

    fputlnf(out, "\n#ifndef STB_%s_H_INCLUDED", library_uc);
    fputlnf(out, "#define STB_%s_H_INCLUDED\n", library_uc);

    if (p_order->version.tag) {
        fprintf(out, "#define STB_%s_VERSION ", library_uc);
        e = put_field_string_constant(out, p_order->version);
        if (e) FREE_RETURN(e);
        fputln(out, "\n");
    }

    fputln(out, "#ifdef __cplusplus");
    fputln(out, "extern \"C\" {");
    fputln(out, "#endif\n");

    fputlnf(out, "#ifndef STB_%s_DEFINITION", library_uc);
    fputln(out, "/// @brief Defines modifier keywords for the definitions. Example value: static inline");
    fputlnf(out, "#define STB_%s_DEFINITION", library_uc);
    fputln(out, "#endif\n");

    // header files
    n = arrlenu(p_order->source_files);
    // possible optimization: compute and cache src_type here so we don't need to recompute it for every file for source files
    for (size_t i = 0; i < n; ++i) {
        if (src_type_detect(p_order->source_files[i]) == src_type_header) {
            embed_file(out, p_order->source_files[i]);
        }
    }

    fputln(out, "#ifdef __cplusplus");
    fputln(out, "}");
    fputln(out, "#endif\n");

    fputlnf(out, "#endif // STB_%s_H_INCLUDED\n", library_uc);

    fputlnf(out, "#ifdef STB_%s_IMPLEMENTATION\n", library_uc);

    // source files
    n = arrlenu(p_order->source_files);
    for (size_t i = 0; i < n; ++i) {
        if (src_type_detect(p_order->source_files[i]) == src_type_source) {
            embed_file(out, p_order->source_files[i]);
        }
    }

    fputlnf(out, "#endif // STB_%s_IMPLEMENTATION", library_uc);

    FREE_RETURN(e);
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
