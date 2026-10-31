#pragma once

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
  -e, --entry=FILE        Public header (a .h input, can be repeated). Headers reachable\n\
                          from an entry through quoted includes are declared in the output.\n\
                          Other headers are only visible to the implementation.\n\
                          Without --entry, every header is declared in the output.\n\
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
