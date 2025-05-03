#include "library_order.h"
#include "lib/stb_ds.h"
#include <stdio.h>

static void print_input_field(char const *name, input_field_t field) {
    printf("%s: ", name);
    switch (field.tag) {
    case input_field_null: puts("(none)"); break;
    case input_field_pathname: printf("$(cat %s)\n", field.pathname); break;
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
