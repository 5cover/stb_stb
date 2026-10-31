# Design notes (historical)

> These are the notes written before the tool was implemented. Some ideas were dropped or changed, and the sections on options, entry points and the template describe the planned design. For what the tool does today, see [README.md](../README.md).


Packages a project as an STB-style header only libary.

## Idea for reimplementation

- for each input file : read, discover include trees (maybe they can be parsed unambiguously as preprocessor tokens) until we hit stdlib or forbidden includes
- collect into two flat arrays in a way that puts parents first: headers and sources
- determine file type by extension. h : C header. hpp etc. C++ header. c : C source. cpp etc. : C++ source. other : inherit language of the including file
- emit template with extern "C" and the likes as necessary.
- test compilation of the resulting file (gcc -fsyntax-only or maybe more to simulate linking)
- if it fails to compile show the errors to the users (if there are duplicated  functions, static mebers, all sort of stuff  that usually relies on multiple translation units being present -- identify common failure points)

trivia

if we stb_stb a program with a main function, it will add the main function to the implementation section as for any other function

i think it can be  run with `gcc library.h -DLIBRARY_IMPLEMENTATION`

## Why

- To simplify the development of STB-style libraries
  - Working with multiple smaller files is easier than one huge file
  - The project can be built non STB-style: useful for development and testing
- To formalize STB-style libairies
- For fun

## How

- Command
- Makefile target

## Options

stb_stb

--f* options with file argument can take `-` for stdin.

argument|description
-|-
--help|show help
--version|show version
positional|library name (required)
psoitional (variadic)|source files. .c, .h, .cpp... determined to be source or header based on gcc's rules. -x is supported to override the automtatic detection by extension.
--author=STR<br>--fauthor=FILE|doxygen author name
--brief=STR<br>--fbrief=FILE|doxygen brief description
--copyright=STRR<br>--fcopyright=FILE|doxygen copyright
--details=STR<br>--fdetails=FILE|details, can be specified as file (for long input)
-v, --myversion=STRR<br>-V, --fmyversion=FILE|doxygen version
-dFILE, --dependency=FILE|(can be present multiple times) defines an user included file as an external dependency, meaning it won't be included in the output. needs not be present in the source files
--no-check|skip -fsyntax check
-o=FILE, --output=FILE|output filename. use `-` for stdout. if not present, defaults to `stb_<library>.h`. if exists as a directory -> creates `stb_<library.h>` in that directory
--overwrite|enables overwriting the output file (otherwise doesn't overwrite it)

### Future

-e, --entry=FILE|(can be present multiple times) adds an entry point header file. argument value can be in the source files. if not present, single header in the source file list (error if multiple). entry points define the public api

## Implementation

replace in template

### options

- validate --library is a valid c identifier and uppercase it to get library
- keep the original --library for filename
- don't include the line at all of missing (e.g no `/// @author` if no --author)
- fdetails: trim single trailing newline
- when generating documentation, replace `\n` by `\n///<space>`
- for version macro, replace `\n` by `\\\n`

## basic flow for mvp

1. parse cli args
2. categorize source files (unknown (warn and ignore), source, header)
3. start building template and documentation
4. put source files in order (no dependency check, no include guard removal for now)
5. test compilation of the resulting file, maybe? (add a --no-check option to disable it -- useful if the user's system doesn't have gcc): what gcc command to use to "test if this file compiles (syntax and semantic errors), but don't actually output a binary"

### reducing code to a single file

give an "entry" point header file

generate {dependencies}: (markdown list) of "user-included" and \<system-included\> files

skip dependencies section if no dependencies: no empty list

- resolving includes?
- error on redefinition (multiple definitions of the same name in the same translation unit). no automatic namespacing to avoid "magic"

usual scenario

- one front header: mylib.h
- included by other headers and source files
- provided file: header + sources

## Template

stb_{library}.h

```c
/// @file
/// @author {AUTHOR}
/// @brief {BRIEF}
/// @copyright {COPYRIGHT}
/// @version {VERSION}
/// @remark To build this, in one source file that includes this file do
/// @code
/// #define STB_{LIBRARY}_IMPLEMENTATION
/// @endcode
/// @remark Dependencies:
/// {DEPENDENCIES}
/// Local paths are relative to this file.
///
/// {DETAILS}

#ifndef STB_{LIBRARY}_H_INCLUDED
#define STB_{LIBRARY}_H_INCLUDED

#define STB_{LIBRARY}_VERSION {VERSION}

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STB_{LIBRARY}_DEFINITION
/// @brief Defines modifier keywords for the definitions. Example value: static inline
#define STB_{LIBRARY}_DEFINITION
#endif

// api includes

// declarations...
// function and variable declarations prepended by STB_{LIBRARY}_DEFINITION

#ifdef __cplusplus
}
#endif

#endif // STB_{LIBRARY}_H_INCLUDED

#ifdef STB_{LIBRARY}_IMPLEMENTATION

// impl include

// implementations...
// function and variable declarations prepended by STB_{LIBRARY}_DEFINITION

#endif // STB_{LIBRARY}_IMPLEMENTATION
```

## Stripping includes: how

Where can #include (or other directives appear):

- The `#` must come at the start of a logical line (ignoring whitespace characters)
- whitespace after `#`
- line continuations may appear anywhere
- any whitespace may appear between `include` and the path

regex

`/(?<!\\\n)\h*#\h*include\h*""/`
