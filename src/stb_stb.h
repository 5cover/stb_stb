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
