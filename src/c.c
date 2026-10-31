#include "lib/c.h"

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
