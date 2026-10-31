#include "lib/c.h"
#include "lib/library_order.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                                  \
    do {                                                                             \
        if (!(cond)) {                                                               \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                              \
        }                                                                            \
    } while (0)

static c_directive_vec_t scan(char const *text) {
    c_directive_vec_t out = { 0 };
    c_scan(text, strlen(text), &out);
    return out;
}

/// @return true if the directive's span is exactly the text of @p line, newline included.
static bool spans(char const *text, c_directive_t const *d, char const *line) {
    size_t const n = strlen(line);
    return d->end - d->begin == n && !memcmp(text + d->begin, line, n);
}

static void test_quoted_and_system_includes(void) {
    char const *text = "#include \"a.h\"\n#include <stdio.h>\n";
    c_directive_vec_t v = scan(text);
    CHECK(v.len == 2);
    CHECK(v.data[0].tag == c_directive_include && v.data[0].is_usr && !strcmp(v.data[0].name, "a.h"));
    CHECK(v.data[1].tag == c_directive_include && !v.data[1].is_usr && !strcmp(v.data[1].name, "stdio.h"));
    CHECK(spans(text, &v.data[0], "#include \"a.h\"\n"));
    CHECK(spans(text, &v.data[1], "#include <stdio.h>\n"));
}

static void test_spacing_and_no_space_forms(void) {
    c_directive_vec_t v = scan("#  include   \"a.h\"\n#include<b.h>\n\t#\tinclude \"c.h\"\n");
    CHECK(v.len == 3);
    CHECK(!strcmp(v.data[0].name, "a.h"));
    CHECK(!strcmp(v.data[1].name, "b.h"));
    CHECK(!strcmp(v.data[2].name, "c.h"));
}

static void test_comments_hide_directives(void) {
    c_directive_vec_t v = scan("/* #include \"x.h\" */\n// #include \"y.h\"\n/*\n#include \"z.h\"\n*/\n");
    CHECK(v.len == 0);
}

static void test_literals_hide_directives_and_comment_markers(void) {
    char const *text = "#define S \"/* #include \\\"n.h\\\"\"\nchar c = '\"';\n#include \"real.h\"\n";
    c_directive_vec_t v = scan(text);
    CHECK(v.len == 1);
    CHECK(!strcmp(v.data[0].name, "real.h"));
}

static void test_line_must_start_with_hash(void) {
    c_directive_vec_t v = scan("int x; #include \"a.h\"\n");
    CHECK(v.len == 0);
}

static void test_splice_joins_lines(void) {
    c_directive_vec_t v = scan("#inc\\\nlude \"a.h\"\n");
    CHECK(v.len == 1);
    CHECK(!strcmp(v.data[0].name, "a.h"));
}

static void test_spliced_define_is_not_a_directive(void) {
    c_directive_vec_t v = scan("#define X \\\n  #include \"a.h\"\n");
    CHECK(v.len == 0);
}

static void test_comment_between_keyword_and_name(void) {
    c_directive_vec_t v = scan("#include /* split\n  comment */ \"b.h\"\nint x;\n");
    CHECK(v.len == 1);
    CHECK(!strcmp(v.data[0].name, "b.h"));
}

static void test_computed_and_unterminated_includes(void) {
    c_directive_vec_t v = scan("#include MACRO\n#include \"open.h\n");
    CHECK(v.len == 2);
    CHECK(v.data[0].tag == c_directive_include_computed);
    CHECK(v.data[1].tag == c_directive_include_computed);
}

static void test_include_next_is_not_include(void) {
    c_directive_vec_t v = scan("#include_next \"a.h\"\n");
    CHECK(v.len == 0);
}

static void test_pragma_once(void) {
    c_directive_vec_t v = scan("#pragma once\n#pragma GCC system_header\n#pragma once_x\n");
    CHECK(v.len == 1);
    CHECK(v.data[0].tag == c_directive_pragma_once);
}

static void test_crlf_line_endings(void) {
    char const *text = "#include \"a.h\"\r\nint x;\r\n";
    c_directive_vec_t v = scan(text);
    CHECK(v.len == 1);
    CHECK(!strcmp(v.data[0].name, "a.h"));
    CHECK(spans(text, &v.data[0], "#include \"a.h\"\r\n"));
}

static void test_last_line_without_newline(void) {
    char const *text = "int x;\n#include \"a.h\"";
    c_directive_vec_t v = scan(text);
    CHECK(v.len == 1);
    CHECK(v.data[0].end == strlen(text));
}

static void test_directive_after_block_comment_on_its_own_line(void) {
    c_directive_vec_t v = scan("/* header */\n#include \"a.h\"\n");
    CHECK(v.len == 1);
}

static void test_src_type_detect(void) {
    CHECK(src_type_detect("a.c") == src_type_source);
    CHECK(src_type_detect("dir/a.h") == src_type_header);
    CHECK(src_type_detect("dir\\a.h") == src_type_header);
    CHECK(src_type_detect("dir.v2/a") == src_type_others);
    CHECK(src_type_detect("a.cpp") == src_type_others);
    CHECK(src_type_detect("a.H") == src_type_others);
    CHECK(src_type_detect("noext") == src_type_others);
}

int main(void) {
    test_quoted_and_system_includes();
    test_spacing_and_no_space_forms();
    test_comments_hide_directives();
    test_literals_hide_directives_and_comment_markers();
    test_line_must_start_with_hash();
    test_splice_joins_lines();
    test_spliced_define_is_not_a_directive();
    test_comment_between_keyword_and_name();
    test_computed_and_unterminated_includes();
    test_include_next_is_not_include();
    test_pragma_once();
    test_crlf_line_endings();
    test_last_line_without_newline();
    test_directive_after_block_comment_on_its_own_line();
    test_src_type_detect();

    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    puts("unit tests passed");
    return 0;
}
