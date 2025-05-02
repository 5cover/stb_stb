/// @file
/// @author Raphaël
/// @brief Memory list - Interface
/// @date 13/04/2025

#ifndef MEMLST_H
#define MEMLST_H

#include <stdbool.h>

// enable this for debuggin
// #define MEMLST_TRACE

#ifdef MEMLST_TRACE
#define FILE_LINE_PARAMS , char const *file, int line
#define FILE_LINE_ARGS , file, line
#else
#define FILE_LINE_PARAMS
#define FILE_LINE_ARGS
#endif

typedef struct memlst memlst_t;

/// @brief Create a new, empty memory list.
/// @return A new memory list.
memlst_t *memlst_init(void);

void dtor_json_object(void *jo);

/// @brief Destroy a memory list. Free all allocated memory, including memory for the memory list itself.
/// @param p_memlst Pointer to a memory list.
/// @remark @p p_memlst points to invalid memory after calling this function.
void memlst_destroy(memlst_t **p_memlst FILE_LINE_PARAMS);

/// @brief A destructor function for memory lists.
typedef void (*dtor_fn)(void *);

/// @brief Add an allocation to a memory list.
/// @param p_memlst Pointer to a memory list.
/// @param dtor The destructor function to add on cleanup.
/// @param ptr The pointer to add.
/// @return @p ptr, or @c NULL or the system is out of memoory.
/// @remark if @p ptr is @c NULL, this is a no-op and other parameters can be @c NULL.
void *memlst_add(memlst_t *restrict *restrict p_memlst, dtor_fn dtor, void *restrict ptr FILE_LINE_PARAMS);

/// @brief Empty a memory list's allocations and runs destructors.
/// @param p_memlst Pointer to a memory list.
void memlst_collect(memlst_t **p_memlst FILE_LINE_PARAMS);

#if !defined MEMLST_IMPL && defined MEMLST_TRACE
#define memlst_destroy(memlst) memlst_destroy(memlst, __FILE__, __LINE__)
#define memlst_add(memlst, dtor, ptr) memlst_add(memlst, dtor, ptr, __FILE__, __LINE__)
#define memlst_collect(memlst) memlst_collect(memlst, __FILE__, __LINE__)
#endif

/// @brief Safely destroys a memory list and returns a status.
///
/// This macro destroys the given memory list and immediately returns the specified status.
/// It ensures proper memory cleanup before returning from a function.
///
/// @param pmem The memory list to be destroyed.
/// @param value The value to return after destroying the memory list.
#define CLEAN_RETURN(pmem, value) \
    do {                         \
        memlst_destroy(pmem);  \
        return (value);          \
    } while (0)

/// @brief Safely destroys a memory list and exits the program.
///
/// This macro destroys the given memory list and immediately exits the program
/// with the specified status. It ensures proper memory cleanup before terminating
/// the program.
///
/// @param pmem The memory list to be destroyed.
/// @param status The exit status value to use when terminating the program.
#define CLEAN_EXIT(pmem, status) \
    do {                        \
        memlst_destroy(pmem); \
        exit(status);           \
    } while (0)

#endif // MEMLST_H
