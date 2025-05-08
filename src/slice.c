/// @file
/// @author Raphaël
/// @brief Slice data structre - Implementation
/// @date 1/02/2025

#include "lib/slice.h"

#include <limits.h>

int slice_leni(slice_t slice) {
    return slice.len > INT_MAX ? INT_MAX : (int)slice.len;
}