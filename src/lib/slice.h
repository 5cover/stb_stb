/// @file
/// @author Raphaël
/// @brief slice data strucutre - Interface
/// @date 1/02/2025

#ifndef SLICE_H
#define SLICE_H

#include <stdlib.h>

/// @brief A read-only memory slice.
typedef struct {
    size_t len;
    /// @remark Make sure to check the length before using this pointer.
    char const *val;
} slice_t;

#define SLICE_CONST(STRLIT) \
    (slice_t) { .len = sizeof (STRLIT) - 1, .val = (STRLIT) }

/// @brief Returns the length of a slice as a signed integer.
/// @param slice A slice.
/// @return The length of @p slice, capped to @ref INT_MAX.
int slice_leni(slice_t slice);

#endif // SLICE_H