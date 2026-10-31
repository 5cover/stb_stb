/// @file
/// @remark To build this, in one source file that includes this file do
/// @code
/// #define STB_STB_IMPLEMENTATION
/// @endcode
/// @remark Dependencies:
/// - <stdlib.h>
/// - <stdbool.h>
/// - <stddef.h>
/// - <ctype.h>
/// - <string.h>
/// - <errno.h>
/// - <stdarg.h>
/// - <stdio.h>
/// - <sys/stat.h>
/// - <getopt.h>

#ifndef STB_STB_H_INCLUDED
#define STB_STB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STB_STB_DEFINITION
/// @brief Defines modifier keywords for the definitions. Example value: static inline
#define STB_STB_DEFINITION
#endif


#include <stdlib.h>

/// @brief Declares a growable array type @p name holding elements of type @p T.
#define VEC_DEFINE(name, T) \
    typedef struct {        \
        T *data;            \
        size_t len, cap;    \
    } name

/// @brief Appends @p x to the vec @p v.
#define vec_push(v, x)                                                    \
    do {                                                                  \
        if ((v)->len == (v)->cap) {                                       \
            (v)->cap = (v)->cap ? 2 * (v)->cap : 16;                      \
            (v)->data = realloc((v)->data, (v)->cap * sizeof *(v)->data); \
        }                                                                 \
        (v)->data[(v)->len++] = (x);                                      \
    } while (0)



#include <stdbool.h>
#include <stddef.h>

typedef struct {
    enum {
        c_directive_include,
        c_directive_include_computed, ///< #include followed by something other than a header name
        c_directive_pragma_once,
    } tag;
    bool is_usr;  ///< include only: "name" rather than <name>
    char *name;   ///< include only: the header name without delimiters
    size_t begin; ///< offset of the '#' in the original text
    size_t end;   ///< offset just past the directive's line, newline included
} c_directive_t;

VEC_DEFINE(c_directive_vec_t, c_directive_t);

/// @brief Appends the directives the packager cares about to @p out.
/// @remark Line splices, comments and string or character literals are handled as in translation phases 1 to 3.
/// @remark Macro expansion and conditional inclusion are not evaluated.
void c_scan(char const *text, size_t len, c_directive_vec_t *out);



#include <stdbool.h>

VEC_DEFINE(str_vec_t, char const *);

typedef struct {
    char const *author, *brief, *copyright, *details, *version;
    char const *library;
    char const *output;
    str_vec_t dependencies; ///< user-included headers declared external with --dependency
    str_vec_t source_files;
    bool overwrite, check;
} library_order_t;

typedef enum {
    src_type_others,
    src_type_source,
    src_type_header,
} src_type_t;

src_type_t src_type_detect(char const *filename);

/// @brief Packages the source files of @p order into a single STB-style header.
/// @return EXIT_SUCCESS or EXIT_FAILURE.
int library_order_deliver(library_order_t const *order);


#include <stdbool.h>
#include <stddef.h>

/// @brief Reads a whole file as text. The path "-" reads standard input.
/// @param path The file to read.
/// @param text Receives the contents, NUL-terminated. Owned by the caller.
/// @param len Receives the length of the contents, excluding the terminator.
/// @return true on success. On failure the cause is reported on stderr.
bool text_read(char const *path, char **text, size_t *len);


#define unreachable() __builtin_unreachable()

/// @brief The program's name
#define PROG "stb_stb"

/// @brief The program's version
#define VERSION "0.1.0"

/// @brief The program's usagestring.
#define USAGE "Usage:\n\
  " PROG " LIBRARY SOURCE... [OPTIONS]"
/// @brief The program's helpstring.
#define HELP USAGE "\n\nPackages a multi-file C project into a single-header STB-style library.\n\
\n\
Positional arguments:\n\
  LIBRARY                 Base name of the generated library (e.g. 'image' gives stb_image.h)\n\
  SOURCE...               Source and header files (.c and .h)\n\
\n\
Documentation options:\n\
  --author=STR            Doxygen author line\n\
  --fauthor=FILE          Read author from file ('-' reads stdin)\n\
\n\
  --brief=STR             Doxygen brief description\n\
  --fbrief=FILE           Read brief from file ('-' reads stdin)\n\
\n\
  --copyright=STR         Doxygen copyright\n\
  --fcopyright=FILE       Read copyright from file ('-' reads stdin)\n\
\n\
  --details=STR           Long-form documentation (appended in a Doxygen comment)\n\
  --fdetails=FILE         Read details from file ('-' reads stdin)\n\
\n\
  -v, --myversion=STR     Version string for the generated library\n\
  -V, --fmyversion=FILE   Read version string from file ('-' reads stdin)\n\
\n\
Other options:\n\
  -d, --dependency=FILE\n\
                          Declare a user-included header as external (can be repeated).\n\
                          The include is kept in the output instead of being embedded.\n\
  -o, --output=FILE       Output location.\n\
                          Use '-' to write to stdout (the output is not checked).\n\
                          If FILE is a directory, writes 'stb_<library>.h' inside it.\n\
                          Otherwise writes to FILE (default: './stb_<library>.h').\n\
                          An existing file is only replaced with --overwrite.\n\
\n\
  --overwrite             Allow overwriting the output file\n\
  --no-check              Skip the compilation check of the generated header\n\
\n\
General options:\n\
  --help                  Show this help message and exit\n\
  --version               Show " PROG " version and exit\n\
\n\
Examples:\n\
  " PROG " image src/image.c src/image.h --brief=\"Image loading library\"\n\
  " PROG " json parser/*.c parser/*.h --fdetails=docs/long.md\n\
\n\
Notes:\n\
  - Only the last occurrence of each metadata option is used.\n\
  - The compiler used for the check is $CC, or clang if CC is not set."

#ifdef __cplusplus
}
#endif

#endif // STB_STB_H_INCLUDED

#ifdef STB_STB_IMPLEMENTATION


#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static bool is_blank(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v';
}

/// Phases 1 and 2: removes backslash-newline splices. origin[k] is the offset in the original text of out[k].
static char *splice(char const *text, size_t len, size_t *origin, size_t *n) {
    char *out = malloc(len + 1);
    size_t k = 0;
    for (size_t i = 0; i < len; ++i) {
        if (text[i] == '\\') {
            size_t j = i + 1;
            if (j < len && text[j] == '\r') ++j;
            if (j < len && text[j] == '\n') {
                i = j;
                continue;
            }
        }
        origin[k] = i;
        out[k++] = text[i];
    }
    origin[k] = len;
    out[k] = '\0';
    *n = k;
    return out;
}

static size_t skip_block_comment(char const *s, size_t n, size_t i) {
    for (i += 2; i + 1 < n && !(s[i] == '*' && s[i + 1] == '/'); ++i);
    return i + 1 < n ? i + 2 : n;
}

/// Skips whitespace and comments, stopping before a newline or the first token.
static size_t skip_blank(char const *s, size_t n, size_t i) {
    for (;;) {
        while (i < n && is_blank(s[i])) ++i;
        if (i + 1 < n && s[i] == '/' && s[i + 1] == '*') {
            i = skip_block_comment(s, n, i);
        } else if (i + 1 < n && s[i] == '/' && s[i + 1] == '/') {
            while (i < n && s[i] != '\n') ++i;
        } else {
            return i;
        }
    }
}

/// Skips a string or character literal starting at s[i]. Stops before a newline if the literal is unterminated.
static size_t skip_literal(char const *s, size_t n, size_t i) {
    char const quote = s[i++];
    while (i < n && s[i] != '\n') {
        char const c = s[i++];
        if (c == '\\') {
            ++i;
        } else if (c == quote) {
            break;
        }
    }
    return i < n ? i : n;
}

static size_t word_end(char const *s, size_t n, size_t i) {
    while (i < n && (isalnum((unsigned char)s[i]) || s[i] == '_')) ++i;
    return i;
}

static bool is_word(char const *s, size_t begin, size_t end, char const *word) {
    return end - begin == strlen(word) && !memcmp(s + begin, word, end - begin);
}

/// Returns the offset just past the line that starts at s[i], comments and literals included.
static size_t line_end(char const *s, size_t n, size_t i) {
    while (i < n && s[i] != '\n') {
        if (s[i] == '"' || s[i] == '\'') {
            i = skip_literal(s, n, i);
        } else if (i + 1 < n && s[i] == '/' && s[i + 1] == '*') {
            i = skip_block_comment(s, n, i);
        } else if (i + 1 < n && s[i] == '/' && s[i + 1] == '/') {
            while (i < n && s[i] != '\n') ++i;
        } else {
            ++i;
        }
    }
    return i < n ? i + 1 : n;
}

static char *copy_name(char const *s, size_t len) {
    char *name = malloc(len + 1);
    memcpy(name, s, len);
    name[len] = '\0';
    return name;
}

/// Parses the directive whose '#' is at s[i]. Returns false for directives the packager does not care about.
static bool parse_directive(char const *s, size_t n, size_t i, c_directive_t *d) {
    size_t const begin = skip_blank(s, n, i + 1);
    size_t const end = word_end(s, n, begin);

    if (is_word(s, begin, end, "include")) {
        size_t const p = skip_blank(s, n, end);
        d->tag = c_directive_include_computed;
        if (p < n && (s[p] == '"' || s[p] == '<')) {
            char const close = s[p] == '"' ? '"' : '>';
            size_t q = p + 1;
            while (q < n && s[q] != close && s[q] != '\n') ++q;
            if (q < n && s[q] == close) {
                d->tag = c_directive_include;
                d->is_usr = close == '"';
                d->name = copy_name(s + p + 1, q - p - 1);
            }
        }
        return true;
    }

    if (is_word(s, begin, end, "pragma")) {
        size_t const once = skip_blank(s, n, end);
        size_t const once_end = word_end(s, n, once);
        if (!is_word(s, once, once_end, "once")) return false;
        d->tag = c_directive_pragma_once;
        return true;
    }

    return false;
}

void c_scan(char const *text, size_t len, c_directive_vec_t *out) {
    size_t *origin = malloc((len + 1) * sizeof *origin);
    size_t n;
    char *s = splice(text, len, origin, &n);

    bool line_start = true;
    size_t i = 0;
    while (i < n) {
        i = skip_blank(s, n, i);
        if (i >= n) break;

        char const c = s[i];
        if (c == '\n') {
            line_start = true;
            ++i;
        } else if (c == '#' && line_start) {
            size_t const next = line_end(s, n, i);
            c_directive_t d = { .begin = origin[i], .end = origin[next] };
            if (parse_directive(s, n, i, &d)) vec_push(out, d);
            i = next;
        } else {
            line_start = false;
            i = (c == '"' || c == '\'') ? skip_literal(s, n, i) : i + 1;
        }
    }

    free(s);
    free(origin);
}


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

static void put_group(FILE *out, source_vec_t const *files, size_vec_t const *seq, src_type_t type) {
    for (size_t j = 0; j < seq->len; ++j) {
        source_t const *s = &files->data[seq->data[j]];
        if (s->type == type) put_body(out, files, s);
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

    put_group(out, files, seq, src_type_header);

    fputs("#ifdef __cplusplus\n}\n#endif\n\n", out);
    fprintf(out, "#endif // STB_%s_H_INCLUDED\n\n", upper);

    fprintf(out, "#ifdef STB_%s_IMPLEMENTATION\n\n", upper);
    put_group(out, files, seq, src_type_source);
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


#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    OPT_HELP = 256,
    OPT_VERSION,
    OPT_AUTHOR,
    OPT_FAUTHOR,
    OPT_BRIEF,
    OPT_FBRIEF,
    OPT_COPYRIGHT,
    OPT_FCOPYRIGHT,
    OPT_DETAILS,
    OPT_FDETAILS,
    OPT_OVERWRITE,
    OPT_NO_CHECK,
};

typedef enum {
    cli_ok,
    cli_help,
    cli_version,
    cli_error,
} cli_result_t;

static bool is_identifier(char const *s) {
    if (!*s || isdigit((unsigned char)*s)) return false;
    for (; *s; ++s) {
        if (!isalnum((unsigned char)*s) && *s != '_') return false;
    }
    return true;
}

/// Loads a metadata field from a file. A single trailing newline is dropped.
static bool load_field(char const *path, char const **field) {
    char *text;
    size_t len;
    if (!text_read(path, &text, &len)) return false;
    if (len && text[len - 1] == '\n') text[--len] = '\0';
    *field = text;
    return true;
}

static cli_result_t parse_cli(int argc, char **argv, library_order_t *order) {
    struct option const longopts[] = {
        { .name = "help", .has_arg = no_argument, .val = OPT_HELP },
        { .name = "version", .has_arg = no_argument, .val = OPT_VERSION },
        { .name = "author", .has_arg = required_argument, .val = OPT_AUTHOR },
        { .name = "fauthor", .has_arg = required_argument, .val = OPT_FAUTHOR },
        { .name = "brief", .has_arg = required_argument, .val = OPT_BRIEF },
        { .name = "fbrief", .has_arg = required_argument, .val = OPT_FBRIEF },
        { .name = "copyright", .has_arg = required_argument, .val = OPT_COPYRIGHT },
        { .name = "fcopyright", .has_arg = required_argument, .val = OPT_FCOPYRIGHT },
        { .name = "details", .has_arg = required_argument, .val = OPT_DETAILS },
        { .name = "fdetails", .has_arg = required_argument, .val = OPT_FDETAILS },
        { .name = "myversion", .has_arg = required_argument, .val = 'v' },
        { .name = "fmyversion", .has_arg = required_argument, .val = 'V' },
        { .name = "dependency", .has_arg = required_argument, .val = 'd' },
        { .name = "output", .has_arg = required_argument, .val = 'o' },
        { .name = "overwrite", .has_arg = no_argument, .val = OPT_OVERWRITE },
        { .name = "no-check", .has_arg = no_argument, .val = OPT_NO_CHECK },
        { 0 },
    };

    *order = (library_order_t) { .check = true };

    int opt;
    while ((opt = getopt_long(argc, argv, "v:V:d:o:", longopts, NULL)) != -1) {
        switch (opt) {
        case OPT_HELP: return cli_help;
        case OPT_VERSION: return cli_version;

        case OPT_AUTHOR: order->author = optarg; break;
        case OPT_FAUTHOR:
            if (!load_field(optarg, &order->author)) return cli_error;
            break;
        case OPT_BRIEF: order->brief = optarg; break;
        case OPT_FBRIEF:
            if (!load_field(optarg, &order->brief)) return cli_error;
            break;
        case OPT_COPYRIGHT: order->copyright = optarg; break;
        case OPT_FCOPYRIGHT:
            if (!load_field(optarg, &order->copyright)) return cli_error;
            break;
        case OPT_DETAILS: order->details = optarg; break;
        case OPT_FDETAILS:
            if (!load_field(optarg, &order->details)) return cli_error;
            break;
        case 'v': order->version = optarg; break;
        case 'V':
            if (!load_field(optarg, &order->version)) return cli_error;
            break;

        case 'd': vec_push(&order->dependencies, optarg); break;
        case 'o': order->output = optarg; break;
        case OPT_OVERWRITE: order->overwrite = true; break;
        case OPT_NO_CHECK: order->check = false; break;

        default:
            fprintf(stderr, "%s\nsee --help for details\n", USAGE);
            return cli_error;
        }
    }

    if (optind + 1 >= argc) {
        fputs(PROG ": error: expected LIBRARY and at least one SOURCE\n", stderr);
        return cli_error;
    }
    order->library = argv[optind];
    if (!is_identifier(order->library)) {
        fputs(PROG ": error: LIBRARY must be a C identifier\n", stderr);
        return cli_error;
    }
    for (int i = optind + 1; i < argc; ++i) vec_push(&order->source_files, argv[i]);

    return cli_ok;
}

int main(int argc, char **argv) {
    library_order_t order;
    switch (parse_cli(argc, argv, &order)) {
    case cli_ok: return library_order_deliver(&order);
    case cli_help: puts(HELP); return EXIT_SUCCESS;
    case cli_version: puts(VERSION); return EXIT_SUCCESS;
    case cli_error: return EXIT_FAILURE;
    }
    unreachable();
}


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

#endif // STB_STB_IMPLEMENTATION
