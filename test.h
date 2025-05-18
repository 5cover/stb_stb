/// @file
/// @remark To build this, in one source file that includes this file do
/// @code
/// #define STB_TESTLIB_IMPLEMENTATION
/// @endcode

#ifndef STB_TESTLIB_H_INCLUDED
#define STB_TESTLIB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STB_TESTLIB_DEFINITION
/// @brief Defines modifier keywords for the definitions. Example value: static inline
#define STB_TESTLIB_DEFINITION
#endif

#pragma once

int add(int a, int b);
#ifdef __cplusplus
}
#endif

#endif // STB_TESTLIB_H_INCLUDED

#ifdef STB_TESTLIB_IMPLEMENTATION

#include "stb_bc.h"

int add(int a, int b) {
    // addition with love
    return a + b;
}

#endif // STB_TESTLIB_IMPLEMENTATION
