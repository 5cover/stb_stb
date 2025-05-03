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

static inline parse_result_t parse_cli(int argc, char **argv, memlst_t **pmem);

int main(int argc, char **argv) {
    memlst_t *mem = memlst_init();
    parse_result_t pr = parse_cli(argc, argv, &mem);

    switch (pr.tag) {
    case parse_result_order:
        library_order_describe(&pr.order);
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

parse_result_t parse_cli(int argc, char **argv, memlst_t **pmem) {
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
        case OPT_FAUTHOR: order.author = input_field_pathname(optarg); break;
        case OPT_BRIEF: order.brief = input_field_string(optarg); break;
        case OPT_FBRIEF: order.brief = input_field_pathname(optarg); break;
        case OPT_COPYRIGHT: order.copyright = input_field_string(optarg); break;
        case OPT_FCOPYRIGHT: order.copyright = input_field_pathname(optarg); break;
        case OPT_DETAILS: order.details = input_field_string(optarg); break;
        case OPT_FDETAILS: order.details = input_field_pathname(optarg); break;
        case OPT_MYVERSION: order.version = input_field_string(optarg); break;
        case OPT_FMYVERSION: order.version = input_field_pathname(optarg); break;

        case OPT_OUTPUT: order.output = optarg; break;
        case OPT_DEPENDENCY:
            if (!order.dependencies) memlst_add(pmem, stbds_arrfreef, order.dependencies);
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
    memlst_add(pmem, stbds_arrfreef, order.source_files);
    do {
        arrput(order.source_files, argv[optind++]);
    } while (optind < argc);

    return (parse_result_t) { .tag = parse_result_order, .order = order };
}
