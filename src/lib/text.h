#pragma once

#include <stdbool.h>
#include <stddef.h>

/// @brief Reads a whole file as text. The path "-" reads standard input.
/// @param path The file to read.
/// @param text Receives the contents, NUL-terminated. Owned by the caller.
/// @param len Receives the length of the contents, excluding the terminator.
/// @return true on success. On failure the cause is reported on stderr.
bool text_read(char const *path, char **text, size_t *len);
