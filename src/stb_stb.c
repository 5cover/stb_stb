#include "lib/stb_stb.h"
#include "lib/library_order.h"
#include "lib/text.h"
#include "lib/vec.h"

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
        { .name = "entry", .has_arg = required_argument, .val = 'e' },
        { .name = "output", .has_arg = required_argument, .val = 'o' },
        { .name = "overwrite", .has_arg = no_argument, .val = OPT_OVERWRITE },
        { .name = "no-check", .has_arg = no_argument, .val = OPT_NO_CHECK },
        { 0 },
    };

    *order = (library_order_t) { .check = true };

    int opt;
    while ((opt = getopt_long(argc, argv, "v:V:d:e:o:", longopts, NULL)) != -1) {
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
        case 'e': vec_push(&order->entries, optarg); break;
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
