/// @file
/// @remark To build this, in one source file that includes this file do
/// @code
/// #define STB_STB_STB_IMPLEMENTATION
/// @endcode

#ifndef STB_STB_STB_H_INCLUDED
#define STB_STB_STB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STB_STB_STB_DEFINITION
/// @brief Defines modifier keywords for the definitions. Example value: static inline
#define STB_STB_STB_DEFINITION
#endif

#pragma once

typedef struct {
    enum {
        input_field_null = 0,
        input_field_string,
        input_field_filename,
    } tag;
    union {
        char const *string;
        char const *filename;
    };
} input_field_t;

_Static_assert(input_field_null == 0, "must be false for boolean use");

#define input_field_filename(value) (input_field_t) { .tag = input_field_filename, .filename = (value) };
#define input_field_string(value) (input_field_t) { .tag = input_field_string, .string = (value) };

#pragma once

#include "input_field.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    input_field_t author, brief, copyright, details, version;
    char const *library;
    char const *output;
    char const **dependencies; ///< stbds array
    char const **source_files; ///< stbds array
    bool overwrite, check;
} library_order_t;

typedef enum {
    src_type_others,
    src_type_source,
    src_type_header,
} src_type_t;

src_type_t src_type_detect(char const *filename);

int library_order_deliver(library_order_t const *p_order, FILE *out);

void library_order_describe(library_order_t const *p_order);

#pragma once

#define unreachable() __builtin_unreachable()

/// @brief The program's name
#define PROG "stb_stb"

/// @brief The program's usagestring.
#define USAGE "Usage:\n\
  " PROG " LIBRARY SOURCE... [OPTIONS]"
/// @brief The program's helpstring.
#define HELP USAGE "\n\nPackages a multi-file C/C++ project into a single-header STB-style library.\n\
\n\
Positional arguments:\n\
  LIBRARY                 Base name of the generated library (e.g. 'image' → stb_image.h)\n\
  SOURCE...               Source and header files (.c, .cpp, .h, .hpp...)\n\
\n\
Documentation options:\n\
  --author=STR            Doxygen author line\n\
  --fauthor=FILE          Read author from file\n\
\n\
  --brief=STR             Doxygen brief description\n\
  --fbrief=FILE           Read brief from file\n\
\n\
  --copyright=STRR        Doxygen copyright\n\
  --fcopyright=FILE       Read copyright from file\n\
\n\
  --details=STR           Long-form documentation (appended in Doxygen comment)\n\
  --fdetails=FILE         Read details from file (use '-' to read from stdin)\n\
\n\
  -v, --myversion=STR     Version string for the generated library\n\
  -V, --fmyversion=FILE   Read version string from file\n\
\n\
Other options:\n\
  -d, --dependency=FILE\n\
                          Mark a user-included file as external (can be repeated)\n\
  -o, --output=FILE       Output location.\n\
                          Use '-' to write to stdout.\n\
                          If FILE is a directory, creates 'stb_<library>.h' inside it.\n\
                          If FILE exists and is a file, fails unless --overwrite is given.\n\
                          If FILE does not exist, writes to './stb_<library>.h'.\n\
\n\
  --overwrite             Allow overwriting the output file\n\
  --no-check              Skip testing compilation of the generated header\n\
\n\
General options:\n\
  --help                  Show this help message and exit\n\
  --version               Show " PROG " version and exit\n\
\n\
Examples:\n\
  " PROG " image src/image.c src/image.h --entry=src/image.h --brief=\"Image loading library\"\n\
  " PROG " json parser/*.c parser/*.h --entry=parser/api.h --fdetails=docs/long.md\n\
\n\
Notes:\n\
  - Only the last occurrence of each metadata option is used.\n\
  - '--f*' variants allow loading long metadata from files or stdin."

#ifdef __cplusplus
}
#endif

#endif // STB_STB_STB_H_INCLUDED

#ifdef STB_STB_STB_IMPLEMENTATION

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

#include "library_order.h"
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

#include "stb_stb.h"
#include "lib/memlst.h"
#include "lib/stb_ds.h"
#include "library_order.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

typedef enum {
    parse_error_usage,
    // todo: add more as needed
} parse_error_t;

typedef struct {
    enum {
        parse_result_order,
        parse_result_error,
        parse_result_show_help,
        parse_result_show_version,
    } tag;
    union {
        library_order_t order;
        parse_error_t error;
    };
} parse_result_t;

static void free_order(void *ptr);

static inline parse_result_t parse_cli(int argc, char **argv);

int main(int argc, char **argv) {
    memlst_t *mem = memlst_init();
    parse_result_t pr = parse_cli(argc, argv);

    switch (pr.tag) {
    case parse_result_order:
        memlst_add(&mem, free_order, &pr.order);
        library_order_deliver(&pr.order, stdout);
        // todo: intepret order..
        CLEAN_RETURN(&mem, EX_OK);
    case parse_result_show_help:
        puts(HELP);
        CLEAN_RETURN(&mem, EX_OK);
    case parse_result_show_version:
        puts(VERSION);
        CLEAN_RETURN(&mem, EX_OK);
    case parse_result_error:
        CLEAN_RETURN(&mem, EX_USAGE);
    default: unreachable();
    }
}

parse_result_t parse_cli(int argc, char **argv) {
    enum {
        // Common options
        OPT_HELP,
        OPT_VERSION,

        // Input field options
        OPT_AUTHOR,
        OPT_FAUTHOR,
        OPT_BRIEF,
        OPT_FBRIEF,
        OPT_COPYRIGHT,
        OPT_FCOPYRIGHT,
        OPT_DETAILS,
        OPT_FDETAILS,
        OPT_FMYVERSION = 'V',
        OPT_MYVERSION = 'v',

        // Other options
        OPT_DEPENDENCY = 'd',
        OPT_OUTPUT = 'o',
        OPT_OVERWRITE,
        OPT_NO_CHECK,
    };

    struct option longopts[] = {
        {
            .name = "help",
            .val = OPT_HELP,
        },
        {
            .name = "version",
            .val = OPT_VERSION,
        },

        {
            .name = "author",
            .val = OPT_AUTHOR,
            .has_arg = required_argument,
        },
        {
            .name = "fauthor",
            .val = OPT_FAUTHOR,
            .has_arg = required_argument,
        },
        {
            .name = "brief",
            .val = OPT_BRIEF,
            .has_arg = required_argument,
        },
        {
            .name = "fbrief",
            .val = OPT_FBRIEF,
            .has_arg = required_argument,
        },
        {
            .name = "copyright",
            .val = OPT_COPYRIGHT,
            .has_arg = required_argument,
        },
        {
            .name = "fcopyright",
            .val = OPT_FCOPYRIGHT,
            .has_arg = required_argument,
        },
        {
            .name = "details",
            .val = OPT_DETAILS,
            .has_arg = required_argument,
        },
        {
            .name = "fdetails",
            .val = OPT_FDETAILS,
            .has_arg = required_argument,
        },
        {
            .name = "myversion",
            .val = OPT_VERSION,
            .has_arg = required_argument,
        },
        {
            .name = "fmyversion",
            .val = OPT_FMYVERSION,
            .has_arg = required_argument,
        },

        {
            .name = "output",
            .val = OPT_OUTPUT,
            .has_arg = required_argument,
        },
        {
            .name = "dependency",
            .val = OPT_DEPENDENCY,
            .has_arg = required_argument,
        },
        {
            .name = "overwrite",
            .val = OPT_OVERWRITE,
        },
        {
            .name = "no-check",
            .val = OPT_NO_CHECK,
        },

        { 0 },
    };

    library_order_t order = {
        .check = true,
    };

    int opt;
    while (-1 != (opt = getopt_long(argc, argv, "V:v:d:o:", longopts, NULL))) {
        switch (opt) {
        case OPT_HELP: return (parse_result_t) { .tag = parse_result_show_help };
        case OPT_VERSION: return (parse_result_t) { .tag = parse_result_show_version };

        case OPT_AUTHOR: order.author = input_field_string(optarg); break;
        case OPT_FAUTHOR: order.author = input_field_filename(optarg); break;
        case OPT_BRIEF: order.brief = input_field_string(optarg); break;
        case OPT_FBRIEF: order.brief = input_field_filename(optarg); break;
        case OPT_COPYRIGHT: order.copyright = input_field_string(optarg); break;
        case OPT_FCOPYRIGHT: order.copyright = input_field_filename(optarg); break;
        case OPT_DETAILS: order.details = input_field_string(optarg); break;
        case OPT_FDETAILS: order.details = input_field_filename(optarg); break;
        case OPT_MYVERSION: order.version = input_field_string(optarg); break;
        case OPT_FMYVERSION: order.version = input_field_filename(optarg); break;

        case OPT_OUTPUT: order.output = optarg; break;
        case OPT_DEPENDENCY:
            arrput(order.dependencies, optarg);
            break;
        case OPT_OVERWRITE: order.overwrite = true; break;
        case OPT_NO_CHECK: order.check = false; break;

        case '?':
            puts(USAGE);
            puts("see --help for details");
            return (parse_result_t) { .tag = parse_result_error, .error = parse_error_usage };
        default:
            unreachable();
        }
    }

    if (optind < argc) {
        order.library = argv[optind++];
    }
    if (optind >= argc) {
        fputs(PROG ": error: expected at least two positional arguments, LIBRARY and at least one source file\n", stderr);
        return (parse_result_t) { .tag = parse_result_error, .error = parse_error_usage };
    }
    do {
        arrput(order.source_files, argv[optind++]);
    } while (optind < argc);

    return (parse_result_t) { .tag = parse_result_order, .order = order };
}

void free_order(void *ptr)
{
    arrfree(((library_order_t*)ptr)->dependencies);
    arrfree(((library_order_t*)ptr)->source_files);
}
#endif // STB_STB_STB_IMPLEMENTATION
