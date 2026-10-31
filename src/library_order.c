#include "lib/library_order.h"
#include "lib/c.h"
#include "lib/text.h"
#include "lib/vec.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    char const *name; ///< as given on the command line
    char *path;       ///< canonical path, identifies the file
    char *text;
    size_t len;
    src_type_t type;
    c_directive_vec_t directives;
    bool visited;
    bool is_public; ///< header reachable from an entry (or any header when there are no entries)
} source_t;

VEC_DEFINE(source_vec_t, source_t);
VEC_DEFINE(size_vec_t, size_t);

static char *format(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int const n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char *s = malloc((size_t)n + 1);
    va_start(args, fmt);
    vsnprintf(s, (size_t)n + 1, fmt, args);
    va_end(args);
    return s;
}

static char *canonical_path(char const *path) {
#ifdef _WIN32
    return _fullpath(NULL, path, 0);
#else
    return realpath(path, NULL);
#endif
}

static bool exists(char const *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static bool is_directory(char const *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static ptrdiff_t source_index(source_vec_t const *files, char const *path) {
    for (size_t i = 0; i < files->len; ++i) {
        if (!strcmp(files->data[i].path, path)) return (ptrdiff_t)i;
    }
    return -1;
}

/// @return the index of the input file a quoted include refers to, or -1 if it is not an input file.
static ptrdiff_t resolve_include(source_vec_t const *files, source_t const *from, char const *name) {
    size_t dir_len = 0;
    for (size_t i = 0; from->path[i]; ++i) {
        if (from->path[i] == '/' || from->path[i] == '\\') dir_len = i;
    }

    char *joined = format("%.*s/%s", (int)dir_len, from->path, name);
    char *path = canonical_path(joined);
    free(joined);
    if (!path) return -1;

    ptrdiff_t const index = source_index(files, path);
    free(path);
    return index;
}

/// A directive is embedded when its content is written by the packager itself, so it is dropped from the output.
static bool is_embedded(source_vec_t const *files, source_t const *s, c_directive_t const *d) {
    if (d->tag == c_directive_pragma_once) return true;
    return d->tag == c_directive_include && d->is_usr && resolve_include(files, s, d->name) >= 0;
}

static bool load_sources(library_order_t const *order, source_vec_t *files) {
    for (size_t i = 0; i < order->source_files.len; ++i) {
        char const *name = order->source_files.data[i];
        src_type_t const type = src_type_detect(name);
        if (type == src_type_others) {
            fprintf(stderr, "warning: %s: not a .c or .h file, ignored\n", name);
            continue;
        }

        char *path = canonical_path(name);
        if (!path) {
            fprintf(stderr, "error: cannot open %s: %s\n", name, strerror(errno));
            return false;
        }
        if (source_index(files, path) >= 0) {
            free(path);
            continue;
        }

        source_t s = { .name = name, .path = path, .type = type };
        if (!text_read(name, &s.text, &s.len)) return false;
        c_scan(s.text, s.len, &s.directives);
        vec_push(files, s);
    }
    return true;
}

/// Appends the files in dependency order: every input file an input file includes comes first.
static void visit(source_vec_t *files, size_t i, size_vec_t *order) {
    source_t *s = &files->data[i];
    if (s->visited) return;
    s->visited = true;

    for (size_t k = 0; k < s->directives.len; ++k) {
        c_directive_t const *d = &s->directives.data[k];
        if (d->tag != c_directive_include || !d->is_usr) continue;
        ptrdiff_t const dep = resolve_include(files, s, d->name);
        if (dep >= 0) visit(files, (size_t)dep, order);
    }
    vec_push(order, i);
}

/// Marks the headers reachable from the entries through quoted includes as public.
/// Without entries, every header is public.
static bool mark_public(library_order_t const *order, source_vec_t *files) {
    if (!order->entries.len) {
        for (size_t i = 0; i < files->len; ++i) files->data[i].is_public = files->data[i].type == src_type_header;
        return true;
    }

    size_vec_t pending = { 0 };
    for (size_t e = 0; e < order->entries.len; ++e) {
        char const *name = order->entries.data[e];
        char *path = canonical_path(name);
        ptrdiff_t const index = path ? source_index(files, path) : -1;
        free(path);
        if (index < 0) {
            fprintf(stderr, "error: --entry %s is not an input file\n", name);
            return false;
        }
        if (files->data[index].type != src_type_header) {
            fprintf(stderr, "error: --entry %s is not a header\n", name);
            return false;
        }
        vec_push(&pending, (size_t)index);
    }

    while (pending.len) {
        source_t *s = &files->data[pending.data[--pending.len]];
        if (s->is_public) continue;
        s->is_public = true;

        for (size_t k = 0; k < s->directives.len; ++k) {
            c_directive_t const *d = &s->directives.data[k];
            if (d->tag != c_directive_include || !d->is_usr) continue;
            ptrdiff_t const dep = resolve_include(files, s, d->name);
            if (dep >= 0 && files->data[dep].type == src_type_header) vec_push(&pending, (size_t)dep);
        }
    }
    return true;
}

static bool is_declared(library_order_t const *order, c_directive_t const *d) {
    for (size_t i = 0; i < order->dependencies.len; ++i) {
        if (!strcmp(order->dependencies.data[i], d->name)) return true;
    }
    return false;
}

static bool same_include(c_directive_t const *a, c_directive_t const *b) {
    return a->is_usr == b->is_usr && !strcmp(a->name, b->name);
}

/// Collects the includes that stay in the output: system headers and declared dependencies.
static bool gather_dependencies(library_order_t const *order, source_vec_t const *files, size_vec_t const *seq, c_directive_vec_t *deps) {
    for (size_t j = 0; j < seq->len; ++j) {
        source_t const *s = &files->data[seq->data[j]];
        for (size_t k = 0; k < s->directives.len; ++k) {
            c_directive_t const *d = &s->directives.data[k];
            if (is_embedded(files, s, d)) continue;

            if (d->tag == c_directive_include_computed) {
                fprintf(stderr, "error: %s: #include must name a header; computed includes are not supported\n", s->name);
                return false;
            }
            if (d->is_usr && !is_declared(order, d)) {
                fprintf(stderr, "error: %s: \"%s\" is not an input file; add it to the sources or declare it with --dependency\n", s->name, d->name);
                return false;
            }

            bool listed = false;
            for (size_t m = 0; m < deps->len && !listed; ++m) listed = same_include(&deps->data[m], d);
            if (!listed) vec_push(deps, *d);
        }
    }
    return true;
}

static char *output_path(library_order_t const *order) {
    if (order->output && !is_directory(order->output)) return format("%s", order->output);
    char const *dir = order->output ? order->output : ".";
    return format("%s/stb_%s.h", dir, order->library);
}

/// Writes a doxygen field. Each line of @p text becomes a "///" line, and @p tag, if set, opens the first one.
static void put_doxygen(FILE *out, char const *tag, char const *text) {
    do {
        size_t const n = strcspn(text, "\n");
        fputs("///", out);
        if (tag) fprintf(out, " @%s", tag);
        tag = NULL;
        if (n) fprintf(out, " %.*s", (int)n, text);
        putc('\n', out);
        text += n;
    } while (*text++);
}

static void put_c_string(FILE *out, char const *s) {
    putc('"', out);
    for (; *s; ++s) {
        switch (*s) {
        case '"':
        case '\\':
            putc('\\', out);
            putc(*s, out);
            break;
        case '\n': fputs("\\n", out); break;
        default: putc(*s, out);
        }
    }
    putc('"', out);
}

/// Writes a source file with its embedded directives removed.
static void put_body(FILE *out, source_vec_t const *files, source_t const *s) {
    size_t pos = 0;
    for (size_t k = 0; k < s->directives.len; ++k) {
        c_directive_t const *d = &s->directives.data[k];
        if (!is_embedded(files, s, d)) continue;
        fwrite(s->text + pos, 1, d->begin - pos, out);
        pos = d->end;
    }
    fwrite(s->text + pos, 1, s->len - pos, out);
    putc('\n', out);
}

/// Public headers are declared for every user of the library.
static bool in_declarations(source_t const *s) {
    return s->type == src_type_header && s->is_public;
}

/// Sources, and headers that are not public, are visible only to the implementation.
static bool in_implementation(source_t const *s) {
    return s->type == src_type_source || (s->type == src_type_header && !s->is_public);
}

static void put_group(FILE *out, source_vec_t const *files, size_vec_t const *seq, bool (*member)(source_t const *)) {
    for (size_t j = 0; j < seq->len; ++j) {
        source_t const *s = &files->data[seq->data[j]];
        if (member(s)) put_body(out, files, s);
    }
}

static void put_library(FILE *out, library_order_t const *order, char const *upper, c_directive_vec_t const *deps, source_vec_t const *files, size_vec_t const *seq) {
    fputs("/// @file\n", out);
    if (order->author) put_doxygen(out, "author", order->author);
    if (order->brief) put_doxygen(out, "brief", order->brief);
    if (order->copyright) put_doxygen(out, "copyright", order->copyright);
    if (order->version) put_doxygen(out, "version", order->version);
    fputs("/// @remark To build this, in one source file that includes this file do\n", out);
    fputs("/// @code\n", out);
    fprintf(out, "/// #define STB_%s_IMPLEMENTATION\n", upper);
    fputs("/// @endcode\n", out);
    if (deps->len) {
        fputs("/// @remark Dependencies:\n", out);
        for (size_t i = 0; i < deps->len; ++i) {
            c_directive_t const *d = &deps->data[i];
            fprintf(out, "/// - %c%s%c\n", d->is_usr ? '"' : '<', d->name, d->is_usr ? '"' : '>');
        }
    }
    if (order->details) put_doxygen(out, NULL, order->details);

    fprintf(out, "\n#ifndef STB_%s_H_INCLUDED\n#define STB_%s_H_INCLUDED\n\n", upper, upper);
    if (order->version) {
        fprintf(out, "#define STB_%s_VERSION ", upper);
        put_c_string(out, order->version);
        fputs("\n\n", out);
    }

    fputs("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n", out);
    fprintf(out, "#ifndef STB_%s_DEFINITION\n", upper);
    fputs("/// @brief Defines modifier keywords for the definitions. Example value: static inline\n", out);
    fprintf(out, "#define STB_%s_DEFINITION\n#endif\n\n", upper);

    put_group(out, files, seq, in_declarations);

    fputs("#ifdef __cplusplus\n}\n#endif\n\n", out);
    fprintf(out, "#endif // STB_%s_H_INCLUDED\n\n", upper);

    fprintf(out, "#ifdef STB_%s_IMPLEMENTATION\n\n", upper);
    put_group(out, files, seq, in_implementation);
    fprintf(out, "#endif // STB_%s_IMPLEMENTATION\n", upper);
}

static bool compile_check(char const *path, char const *upper) {
    char const *cc = getenv("CC");
    char *cmd = format("%s -fsyntax-only -x c -DSTB_%s_IMPLEMENTATION \"%s\"", cc ? cc : "clang", upper, path);
    int const status = system(cmd);
    free(cmd);
    if (status != 0) fprintf(stderr, "error: %s does not compile\n", path);
    return status == 0;
}

int library_order_deliver(library_order_t const *order) {
    source_vec_t files = { 0 };
    if (!load_sources(order, &files)) return EXIT_FAILURE;
    if (!files.len) {
        fputs("error: no .c or .h source files given\n", stderr);
        return EXIT_FAILURE;
    }

    size_vec_t seq = { 0 };
    for (size_t i = 0; i < files.len; ++i) visit(&files, i, &seq);
    if (!mark_public(order, &files)) return EXIT_FAILURE;

    c_directive_vec_t deps = { 0 };
    if (!gather_dependencies(order, &files, &seq, &deps)) return EXIT_FAILURE;

    char *upper = format("%s", order->library);
    for (char *p = upper; *p; ++p) *p = (char)toupper((unsigned char)*p);

    bool const to_stdout = order->output && !strcmp(order->output, "-");
    char *path = to_stdout ? NULL : output_path(order);
    if (path && !order->overwrite && exists(path)) {
        fprintf(stderr, "error: %s exists, use --overwrite to replace it\n", path);
        return EXIT_FAILURE;
    }

    FILE *out = to_stdout ? stdout : fopen(path, "wb");
    if (!out) {
        fprintf(stderr, "error: cannot write %s: %s\n", path, strerror(errno));
        return EXIT_FAILURE;
    }
    put_library(out, order, upper, &deps, &files, &seq);
    if (to_stdout) return EXIT_SUCCESS;

    fclose(out);
    return !order->check || compile_check(path, upper) ? EXIT_SUCCESS : EXIT_FAILURE;
}
