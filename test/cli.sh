#!/bin/sh
# Command-line tests for stb_stb.
# Usage: sh test/cli.sh path/to/stb_stb
# Needs a C compiler on PATH (clang by default, or $CC) for the compile check.

set -u

tool_arg=${1:?usage: sh test/cli.sh path/to/stb_stb}
cd "$(dirname "$0")/.." || exit 1
root=$(pwd)
tool=$root/$tool_arg
work=$root/bin/cli-test
rm -rf "$work"
mkdir -p "$work" || exit 1
failures=0

pass() {
    echo "ok: $1"
}

fail() {
    echo "FAIL: $*"
    failures=$((failures + 1))
}

# run NAME WANT_STATUS COMMAND...
# Runs COMMAND, keeping its output in $work/NAME.out and $work/NAME.err.
run() {
    name=$1
    want=$2
    shift 2
    "$@" >"$work/$name.out" 2>"$work/$name.err"
    got=$?
    if [ "$got" -eq "$want" ]; then
        pass "$name"
    else
        fail "$name: exit status $got, want $want"
        sed 's/^/    stderr: /' "$work/$name.err"
    fi
}

# has FILE TEXT NAME: FILE contains TEXT as a fixed string.
has() {
    if grep -qF -- "$2" "$1"; then
        pass "$3"
    else
        fail "$3: '$2' not found in $(basename "$1")"
    fi
}

# lacks FILE TEXT NAME: FILE does not contain TEXT.
lacks() {
    if grep -qF -- "$2" "$1"; then
        fail "$3: '$2' should not be in $(basename "$1")"
    else
        pass "$3"
    fi
}

# before FILE FIRST SECOND NAME: the first line containing FIRST comes before the first line containing SECOND.
before() {
    a=$(grep -nF -- "$2" "$1" | head -n 1 | cut -d: -f1)
    b=$(grep -nF -- "$3" "$1" | head -n 1 | cut -d: -f1)
    if [ -n "$a" ] && [ -n "$b" ] && [ "$a" -lt "$b" ]; then
        pass "$4"
    else
        fail "$4: '$2' (line $a) should come before '$3' (line $b) in $(basename "$1")"
    fi
}

# write PATH: creates PATH (and its directory) from standard input.
write() {
    mkdir -p "$(dirname "$1")" && cat >"$1"
}

d=$work/fixtures

# ---------------------------------------------------------------- usage

run help 0 "$tool" --help
has "$work/help.out" "Usage:" "help prints usage"

run version 0 "$tool" --version
has "$work/version.out" "0.1.0" "version prints the version"

run no-arguments 1 "$tool"
run only-library 1 "$tool" demo
run bad-library-name 1 "$tool" 9demo "$root/test/add.c"
has "$work/bad-library-name.err" "C identifier" "bad library name is explained"
run unknown-option 1 "$tool" demo --bogus "$root/test/add.c"
run missing-source 1 "$tool" demo "$d/nope.c" -o "$work/nope.h"
has "$work/missing-source.err" "cannot open" "missing source is explained"

# ---------------------------------------------------------------- basic packaging

run basic 0 "$tool" demo "$root/test/add.c" "$root/test/stb_bc.h" -o "$work/basic.h"
has "$work/basic.h" "#define STB_DEMO_IMPLEMENTATION" "basic: implementation macro is named after the library"
has "$work/basic.h" "#ifndef STB_DEMO_H_INCLUDED" "basic: include guard is generated"
has "$work/basic.h" "int add(int a, int b);" "basic: header declarations are embedded"
has "$work/basic.h" "int add(int a, int b) {" "basic: source definitions are embedded"
lacks "$work/basic.h" "#include \"stb_bc.h\"" "basic: internal quoted include is removed"
lacks "$work/basic.h" "#pragma once" "basic: #pragma once is removed"
before "$work/basic.h" "int add(int a, int b);" "#ifdef STB_DEMO_IMPLEMENTATION" "basic: declarations come before the implementation section"

mkdir -p "$work/outdir"
run directory-output 0 "$tool" demo "$root/test/add.c" "$root/test/stb_bc.h" -o "$work/outdir"
if [ -f "$work/outdir/stb_demo.h" ]; then pass "directory output writes stb_<library>.h inside it"; else fail "directory output: stb_demo.h not created"; fi

mkdir -p "$work/cwd"
(cd "$work/cwd" && run default-output 0 "$tool" demo "$root/test/add.c" "$root/test/stb_bc.h")
if [ -f "$work/cwd/stb_demo.h" ]; then pass "default output is ./stb_<library>.h"; else fail "default output: stb_demo.h not created in the working directory"; fi

run existing-output 1 "$tool" demo "$root/test/add.c" "$root/test/stb_bc.h" -o "$work/basic.h"
has "$work/existing-output.err" "use --overwrite" "existing output is not replaced without --overwrite"
run overwrite 0 "$tool" demo "$root/test/add.c" "$root/test/stb_bc.h" -o "$work/basic.h" --overwrite

run stdout 0 "$tool" demo "$root/test/add.c" "$root/test/stb_bc.h" -o -
has "$work/stdout.out" "#ifdef STB_DEMO_IMPLEMENTATION" "stdout: output is written to standard output"

run duplicate-input 0 "$tool" demo "$root/test/add.c" "$root/test/add.c" "$root/test/stb_bc.h" -o "$work/dup.h"
if [ "$(grep -c -F -- 'int add(int a, int b) {' "$work/dup.h")" -eq 1 ]; then pass "duplicate input is embedded once"; else fail "duplicate input is embedded more than once"; fi

# ---------------------------------------------------------------- ordering and includes

write "$d/order/a.h" <<'EOF'
#pragma once
#include "b.h"
int a_marker;
EOF
write "$d/order/b.h" <<'EOF'
int b_marker;
EOF

run dependency-first 0 "$tool" order "$d/order/a.h" "$d/order/b.h" -o "$work/order.h"
before "$work/order.h" "int b_marker;" "int a_marker;" "headers are ordered so included headers come first"

run reverse-input 0 "$tool" order "$d/order/b.h" "$d/order/a.h" -o "$work/order-rev.h"
before "$work/order-rev.h" "int b_marker;" "int a_marker;" "input order does not change dependency order"

write "$d/cycle/a.h" <<'EOF'
#include "b.h"
int cycle_a;
EOF
write "$d/cycle/b.h" <<'EOF'
#include "a.h"
int cycle_b;
EOF
run include-cycle 0 "$tool" cycle "$d/cycle/a.h" "$d/cycle/b.h" -o "$work/cycle.h"
has "$work/cycle.h" "int cycle_b;" "include cycle: both headers are embedded"

write "$d/sys/uses_system.c" <<'EOF'
#include <stdio.h>
#include "uses_system.h"
int uses_system(void) { return puts("hi") >= 0; }
EOF
write "$d/sys/uses_system.h" <<'EOF'
#include <stdlib.h>
int uses_system(void);
EOF
run system-includes 0 "$tool" sys "$d/sys/uses_system.h" "$d/sys/uses_system.c" -o "$work/sys.h"
has "$work/sys.h" "/// - <stdlib.h>" "system includes are listed as dependencies"
has "$work/sys.h" "/// - <stdio.h>" "system includes from sources are listed as dependencies"
has "$work/sys.h" "#include <stdlib.h>" "system include is kept in the output"
has "$work/sys.h" "#include <stdio.h>" "system include from a source is kept in the output"
lacks "$work/sys.h" "#include \"uses_system.h\"" "embedded quoted include is removed from a source"

write "$d/ext/uses_external.h" <<'EOF'
#include "external.h"
int uses_external(void);
EOF
write "$d/ext/external.h" <<'EOF'
int external_value;
EOF
run undeclared-include 1 "$tool" ext "$d/ext/uses_external.h" -o "$work/ext-fail.h"
has "$work/undeclared-include.err" "not an input file" "undeclared quoted include is an error"

write "$work/external.h" <<'EOF'
int external_value;
EOF
run declared-dependency 0 "$tool" ext "$d/ext/uses_external.h" --dependency=external.h -o "$work/ext.h"
has "$work/ext.h" "#include \"external.h\"" "declared dependency is kept as an include"
has "$work/ext.h" "/// - \"external.h\"" "declared dependency is listed"

write "$d/conditional/a.h" <<'EOF'
#if 0
#include "never_built.h"
#endif
int conditional_a;
EOF
run conditional-include 1 "$tool" conditional "$d/conditional/a.h" -o "$work/conditional.h"
has "$work/conditional-include.err" "not an input file" "limitation: includes in #if 0 are still followed"

write "$d/computed/a.h" <<'EOF'
#define HEADER "other.h"
#include HEADER
EOF
run computed-include 1 "$tool" computed "$d/computed/a.h" -o "$work/computed.h"
has "$work/computed-include.err" "computed includes are not supported" "computed include is an error"

# ---------------------------------------------------------------- source text handling

write "$d/text/crlf.c" <<'EOF'
int crlf_value(void) { return 1; }
EOF
printf '#include "crlf.h"\r\nint crlf_main(void) { return crlf_value(); }\r\n' >"$d/text/crlf_main.c"
printf 'int crlf_value(void);\r\n' >"$d/text/crlf.h"
run crlf-sources 0 "$tool" crlf "$d/text/crlf.h" "$d/text/crlf_main.c" "$d/text/crlf.c" -o "$work/crlf.h"
if grep -q "$(printf '\r')" "$work/crlf.h"; then fail "crlf: output contains carriage returns"; else pass "crlf: output uses LF line endings"; fi
has "$work/crlf.h" "int crlf_main(void)" "crlf: CRLF source is embedded"

write "$d/text/tricky.h" <<'EOF'
/* #include "missing.h" in a comment */
#define QUOTE "/* not a comment"
#define SPLIT \
  #include "missing2.h"
int tricky_value;
EOF
run comment-and-literal-text 0 "$tool" tricky "$d/text/tricky.h" -o "$work/tricky.h"
has "$work/tricky.h" "int tricky_value;" "comments and literals do not produce includes"

# ---------------------------------------------------------------- metadata

printf 'First line\n\nSecond line\n' >"$work/details.txt"
run metadata 0 "$tool" demo "$root/test/add.c" "$root/test/stb_bc.h" \
    --author="Jane Doe" --brief="Adds numbers" --copyright="2026 ACME" \
    --myversion='1.2 "beta"' --fdetails="$work/details.txt" -o "$work/meta.h"
has "$work/meta.h" "/// @author Jane Doe" "metadata: author"
has "$work/meta.h" "/// @brief Adds numbers" "metadata: brief"
has "$work/meta.h" "/// @copyright 2026 ACME" "metadata: copyright"
has "$work/meta.h" '/// @version 1.2 "beta"' "metadata: version doc line"
has "$work/meta.h" '#define STB_DEMO_VERSION "1.2 \"beta\""' "metadata: version macro escapes quotes"
has "$work/meta.h" "/// First line" "metadata: details first line"
has "$work/meta.h" "/// Second line" "metadata: details second line"
if [ "$(grep -A1 -F -- '/// Second line' "$work/meta.h" | sed -n 2p)" = "" ]; then
    pass "metadata: one trailing newline is trimmed from file input"
else
    fail "metadata: details file trailing newline was not trimmed"
fi

printf 'Read from stdin\n' | "$tool" demo "$root/test/add.c" "$root/test/stb_bc.h" --fbrief=- -o "$work/stdin.h" --overwrite
if [ $? -eq 0 ]; then has "$work/stdin.h" "/// @brief Read from stdin" "metadata: '-' reads standard input"; else fail "metadata: '-' stdin run failed"; fi

# ---------------------------------------------------------------- compile check

write "$d/check/broken.c" <<'EOF'
int broken( { }
EOF
run broken-source 1 "$tool" check "$d/check/broken.c" -o "$work/broken.h"
has "$work/broken-source.err" "does not compile" "compile check reports failure"
if [ -f "$work/broken.h" ]; then pass "broken source: header is still written"; else fail "broken source: header was not written"; fi

run no-check 0 "$tool" check "$d/check/broken.c" -o "$work/broken-nocheck.h" --no-check

write "$d/check/unknown.cpp" <<'EOF'
int cpp_only;
EOF
run unknown-extension-only 1 "$tool" check "$d/check/unknown.cpp" -o "$work/unknown.h"
has "$work/unknown-extension-only.err" "no .c or .h" "only unknown files is an error"
run unknown-extension-ignored 0 "$tool" check "$root/test/add.c" "$root/test/stb_bc.h" "$d/check/unknown.cpp" -o "$work/unknown-ok.h"
has "$work/unknown-extension-ignored.err" "not a .c or .h file" "unknown file is ignored with a warning"

# ---------------------------------------------------------------- self-packaging

run self-package 0 "$tool" stb "$root"/src/*.c "$root"/src/lib/*.h -o "$work/self.h"
run self-compile 0 clang -std=gnu17 -Werror -DSTB_STB_IMPLEMENTATION -x c "$work/self.h" -o "$work/self-tool"
run self-run 0 "$work/self-tool" --version
has "$work/self-run.out" "0.1.0" "self-packaged tool runs"

# ---------------------------------------------------------------- result

if [ "$failures" -ne 0 ]; then
    echo "$failures CLI test(s) failed"
    exit 1
fi
echo "CLI tests passed"
