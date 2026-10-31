# stb_stb

stb_stb packages a multi-file C project into a single STB-style header. You give it the library name and the project's `.c` and `.h` files, and it writes `stb_<library>.h`. Users of the library include that one file, and in exactly one source file they define `STB_<LIBRARY>_IMPLEMENTATION` before including it.

stb_stb does not compile your code and does not understand it. It reads the `#include` and `#pragma once` directives, copies the text of your files into the right places, and writes the header. The only compilation it runs is an optional syntax check on the result.

Status: early. It is tested on Windows 11 with LLVM-MinGW clang (clang 23.1). Linux and macOS are not tested.

## Quick start

```sh
just build
bin/stb_stb.exe demo test/add.c test/stb_bc.h -o demo.h      # bin/stb_stb on Linux and macOS
```

`demo.h` now contains the declarations and the implementation of `test/add.c` and `test/stb_bc.h`. Use it like any STB-style library:

```c
/* in exactly one .c file */
#define STB_DEMO_IMPLEMENTATION
#include "demo.h"

/* in every other file */
#include "demo.h"
```

## Requirements

- A C compiler for the syntax check that runs after packaging. It is `clang` by default, or the program named by the `CC` environment variable. Use `--no-check` to skip it.
- `just` (optional, for the recipes below). On Windows, `winget install --id Casey.Just --exact`. The recipes run under `sh`, so Git for Windows must be on `PATH`.

## Building and testing

| Command | What it does |
|---|---|
| `just build` | Compiles the tool to `bin/stb_stb` (`bin/stb_stb.exe` on Windows). |
| `just test` | Builds, then runs the unit tests (`test/unit.c`) and the command-line tests (`test/cli.sh`). |
| `just stb` | Packages the tool's own sources into `stb_stb.h`, rebuilds the tool from that header, and checks that it reproduces the same header. |
| `just clean` | Removes `bin/`. |

Without `just`, the build command is:

```sh
clang -std=gnu17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Werror -g -Og src/*.c -o bin/stb_stb
```

## Usage

```
stb_stb LIBRARY SOURCE... [OPTIONS]
```

- `LIBRARY` must be a C identifier. It is used for the file name (`stb_<LIBRARY>.h`) and, in upper case, for the macros (`STB_<LIBRARY>_IMPLEMENTATION`, `STB_<LIBRARY>_H_INCLUDED`, `STB_<LIBRARY>_VERSION`, `STB_<LIBRARY>_DEFINITION`).
- `SOURCE...` are files with the extension `.c` or `.h`. Other files are ignored with a warning. Files can be given in any order: the tool orders them by their includes.

| Option | Meaning |
|---|---|
| `--author=STR`, `--fauthor=FILE` | Doxygen `@author` line. |
| `--brief=STR`, `--fbrief=FILE` | Doxygen `@brief` line. |
| `--copyright=STR`, `--fcopyright=FILE` | Doxygen `@copyright` line. |
| `--details=STR`, `--fdetails=FILE` | Long description, one `///` line per line of text. |
| `-v, --myversion=STR`, `-V, --fmyversion=FILE` | Version. Written as a `@version` doc line and as `STB_<LIBRARY>_VERSION`. |
| `-d, --dependency=NAME` | Declares a quoted include as external. It stays in the output as an `#include`, and the user must provide it. Can be repeated. |
| `-e, --entry=FILE` | Public header (a `.h` input). Can be repeated. Headers reachable from an entry through quoted includes are declared in the output. The others are visible only to the implementation. See [Entry headers](#entry-headers). |
| `-o, --output=FILE` | Output file. A directory gets `stb_<LIBRARY>.h` inside it. `-` writes to standard output, which skips the check. |
| `--overwrite` | Allows replacing an existing output file. |
| `--no-check` | Skips the syntax check of the generated header. |
| `--help`, `--version` | Print help or the tool version and exit. |

The `--f*` options read the value from a file, and `-` reads standard input. One trailing newline is removed from file input. Only the last occurrence of a metadata option is used. `--dependency` accumulates.

The default output is `./stb_<LIBRARY>.h`. If the output path already exists, the tool refuses to write unless `--overwrite` is given. The tool exits with status 0 on success and 1 on any error, including a failed check. On a failed check the header is still written, so you can inspect it.

### Entry headers

An entry header is the front door of the library: the file users include. With `--entry=FILE` (repeatable), the public API is the set of headers reachable from the entries through quoted includes. Only those headers are declared in the output. Every other header is emitted in the implementation section, so the `.c` files still see its declarations, but users of the library do not.

```sh
stb_stb mylib src/api.h src/internal.h src/api.c src/internal.c --entry=src/api.h
```

Without `--entry`, every header is public. An entry must be one of the inputs, and it must be a `.h` file. Entries are matched by the file they name, not by the text of the include that refers to them.

## What the generated header looks like

```
doc block: @file, metadata, "Dependencies" list, details
#ifndef STB_<LIB>_H_INCLUDED / #define ...
#define STB_<LIB>_VERSION "..."         (only with --myversion)
extern "C" wrapper, STB_<LIB>_DEFINITION default
    declarations: public headers (all headers without --entry), dependencies first
#endif
#ifdef STB_<LIB>_IMPLEMENTATION
    implementation: non-public headers and every .c input, dependencies first
#endif
```

Everything declared in the first section is visible to every user of the library. The `.c` files and the non-public headers sit behind `STB_<LIB>_IMPLEMENTATION`.

## How the packaging works

1. Each input file is read and scanned for preprocessor directives. Comments, string and character literals, and backslash-newline splices are handled as the C standard describes in translation phases 1 to 3.
2. A quoted include (`#include "x.h"`) is resolved relative to the directory of the file that contains it. If it names another input file, the file is embedded and the include line is removed. If it names no input file, the tool fails unless the name is declared with `--dependency`.
3. Input files are ordered so that each file comes after the input files it includes. Ties keep the command-line order.
4. Public headers (see [Entry headers](#entry-headers)) go into the declaration section. Sources and the other headers go into the implementation section. Each section keeps the dependency order. `#pragma once` lines are removed, because the output is compiled as a main file during the check.
5. The header is written. Unless `--no-check` is given or the output is stdout, it is compiled with `$CC -fsyntax-only -x c -DSTB_<LIB>_IMPLEMENTATION`.

System includes (`#include <...>`) are kept where they are, and every kept include is listed in the `Dependencies` section of the doc block.

## Supported

- Plain C files with `.c` and `.h` extensions, with LF or CRLF line endings.
- Quoted includes of other input files, at any depth and in any command-line order.
- Include cycles between input files. Each file is embedded once.
- Angle-bracket includes, which are kept as they are.
- Quoted includes declared with `--dependency`, which are kept as they are.
- `#pragma once` in input files (removed) and ordinary include guards (kept).
- Directives written with spaces or tabs between `#` and the keyword, and a directive split across lines with a backslash, or with a comment between the keyword and the name.
- Comments and string or character literals that contain `#include`. These are not directives.
- Duplicate input files (each is embedded once).
- Restricting the declared API with `--entry`, so headers that are not reachable from an entry are visible only to the implementation.
- Metadata text containing quotes, backslashes or newlines. Quotes and backslashes are escaped in the version macro.

## Not supported and known limitations

stb_stb is a packager. It does not preprocess, parse or analyse your code. Read these before relying on it.

- **No preprocessing.** Macros are not expanded and `#if`, `#ifdef`, `#elif` and `#else` are not evaluated. Every `#include` is treated as active, including those in `#if 0` blocks. An include in an inactive block that names a missing header is therefore an error. Test case: `conditional-include` in `test/cli.sh`.
- **Computed includes are errors.** `#include MACRO` and `#include` with any operand other than a header name cannot be resolved, so the tool stops with an error.
- **No include search paths.** A quoted include is resolved only relative to the including file. Anything else needs `--dependency` and must exist next to the output file, or the check fails.
- **Only C.** `.cpp`, `.hpp`, `.H` and other extensions are ignored. The check compiles the header as C.
- **No semantic checks.** Duplicate definitions across files, `static` name clashes, and undeclared identifiers are not detected by the tool. Only the syntax check runs, and only when it is enabled. It is `-fsyntax-only`, so it does not link, and it does not treat warnings as errors.
- **Public headers must be safe in every translation unit.** Declarations, types, macros, `static` functions and `inline` functions are fine. A non-inline function definition or a variable definition in a public header is emitted in the declaration section, so including the library from two `.c` files gives duplicate symbols at link time.
- **All sources share one translation unit.** Macros defined in one source file are visible in every later one, and two `static` functions with the same name in different files clash.
- **Stb-style headers with their own implementation block cannot be embedded.** A third-party header whose implementation is behind its own `*_IMPLEMENTATION` macro is never compiled, because its macro is defined after its declarations. Keep such headers as `--dependency`, or do not package them.
- **Not implemented from the original design notes:** overriding the language by extension (`-x`), and C++. The design notes also described an error when several headers are given without `--entry`; the tool instead makes every header public, so that case is not an error.
- **Paths.** On Windows, paths are normalised with `_fullpath`, which does not resolve symbolic links and compares paths case-sensitively. On POSIX, `realpath` is used. The POSIX branch has not been tested.
- **Whole files are read into memory,** with no size limit.
- **Metadata text is not processed.** Markdown and doxygen commands are copied as they are. Newlines become separate `///` lines.
- **The compile check uses the default compiler settings.** It cannot be told which include directories, defines or language standard to use, apart from the `CC` variable. An angle-bracket include that needs an `-I` directory fails the check, although the header may be correct in your project.

## Tests

`just test` runs two suites:

- `test/unit.c` covers the directive scanner and the file-type detection. Without `just`, build it with `clang -Isrc test/unit.c src/c.c src/src_type.c -o bin/unit` and run `bin/unit`.
- `test/cli.sh` runs the tool on fixtures and checks exit statuses, the generated text, the order of declarations, the error messages, metadata output, the compile check, and the self-packaged tool. It takes the path to the tool as its argument, and writes its scratch files under `bin/cli-test/`.

A test named after a limitation (for example `conditional-include`) documents that behaviour, so it is checked and not assumed.

## Layout

- `src/stb_stb.c`: command-line parsing and `main`.
- `src/library_order.c`: file loading, ordering, entry handling, dependency checks, output and the compile check.
- `src/c.c`, `src/lib/c.h`: the directive scanner.
- `src/src_type.c`: file-type detection by extension.
- `src/text.c`, `src/lib/text.h`: whole-file reading.
- `src/lib/vec.h`: a small growable array.
- `src/lib/stb_stb.h`: the help and usage text, and the version.
- `stb_stb.h`: the tool packaged as a library, generated by `just stb`.
- `docs/design-notes.md`: the original design notes, kept for history.

## Why

- To simplify the development of STB-style libraries.
  - Working with multiple smaller files is easier than one huge file.
  - The project can be built non-STB-style, which is useful for development and testing.
- To formalise STB-style libraries.
- For fun.
