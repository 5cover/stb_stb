/// @file
/// @remark To build this, in one source file that includes this file do
/// @code
/// #define STB_STB_STB_IMPLEMENTATION
/// @endcode
/// @remark Dependencies:
/// - lib/stb_ds.h

#ifndef STB_STB_STB_H_INCLUDED
#define STB_STB_STB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STB_STB_STB_DEFINITION
/// @brief Defines modifier keywords for the definitions. Example value: static inline
#define STB_STB_STB_DEFINITION
#endif

#pragma once

#include "slice.h"
#include <stdbool.h>

slice_t extract_include(char const *line);
#pragma once

typedef struct {
    enum {
        input_field_null = 0,
        input_field_string,
        input_field_filename,
    } tag;
    union {
        char const *string;
        char const *filename;
    };
} input_field_t;

_Static_assert(input_field_null == 0, "must be false for boolean use");

#define input_field_filename(value) (input_field_t) { .tag = input_field_filename, .filename = (value) };
#define input_field_string(value) (input_field_t) { .tag = input_field_string, .string = (value) };

#pragma once

#include "input_field.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    input_field_t author, brief, copyright, details, version;
    char const *library;
    char const *output;
    char const **dependencies; ///< stbds array
    char const **source_files; ///< stbds array
    bool overwrite, check;
} library_order_t;

typedef enum {
    src_type_others,
    src_type_source,
    src_type_header,
} src_type_t;

src_type_t src_type_detect(char const *filename);

int library_order_deliver(library_order_t const *p_order, FILE *out);

void library_order_describe(library_order_t const *p_order);


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
/* stb_ds.h - v0.67 - public domain data structures - Sean Barrett 2019

   This is a single-header-file library that provides easy-to-use
   dynamic arrays and hash tables for C (also works in C++).

   For a gentle introduction:
      http://nothings.org/stb_ds

   To use this library, do this in *one* C or C++ file:
      #define STB_DS_IMPLEMENTATION
      #include "stb_ds.h"

TABLE OF CONTENTS

  Table of Contents
  Compile-time options
  License
  Documentation
  Notes
  Notes - Dynamic arrays
  Notes - Hash maps
  Credits

COMPILE-TIME OPTIONS

  #define STBDS_NO_SHORT_NAMES

     This flag needs to be set globally.

     By default stb_ds exposes shorter function names that are not qualified
     with the "stbds_" prefix. If these names conflict with the names in your
     code, define this flag.

  #define STBDS_SIPHASH_2_4

     This flag only needs to be set in the file containing #define STB_DS_IMPLEMENTATION.

     By default stb_ds.h hashes using a weaker variant of SipHash and a custom hash for
     4- and 8-byte keys. On 64-bit platforms, you can define the above flag to force
     stb_ds.h to use specification-compliant SipHash-2-4 for all keys. Doing so makes
     hash table insertion about 20% slower on 4- and 8-byte keys, 5% slower on
     64-byte keys, and 10% slower on 256-byte keys on my test computer.

  #define STBDS_REALLOC(context,ptr,size) better_realloc
  #define STBDS_FREE(context,ptr)         better_free

     These defines only need to be set in the file containing #define STB_DS_IMPLEMENTATION.

     By default stb_ds uses stdlib realloc() and free() for memory management. You can
     substitute your own functions instead by defining these symbols. You must either
     define both, or neither. Note that at the moment, 'context' will always be NULL.
     @TODO add an array/hash initialization function that takes a memory context pointer.

  #define STBDS_UNIT_TESTS

     Defines a function stbds_unit_tests() that checks the functioning of the data structures.

  Note that on older versions of gcc (e.g. 5.x.x) you may need to build with '-std=c++0x'
     (or equivalentally '-std=c++11') when using anonymous structures as seen on the web
     page or in STBDS_UNIT_TESTS.

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

DOCUMENTATION

  Dynamic Arrays

    Non-function interface:

      Declare an empty dynamic array of type T
        T* foo = NULL;

      Access the i'th item of a dynamic array 'foo' of type T, T* foo:
        foo[i]

    Functions (actually macros)

      arrfree:
        void arrfree(T*);
          Frees the array.

      arrlen:
        ptrdiff_t arrlen(T*);
          Returns the number of elements in the array.

      arrlenu:
        size_t arrlenu(T*);
          Returns the number of elements in the array as an unsigned type.

      arrpop:
        T arrpop(T* a)
          Removes the final element of the array and returns it.

      arrput:
        T arrput(T* a, T b);
          Appends the item b to the end of array a. Returns b.

      arrins:
        T arrins(T* a, int p, T b);
          Inserts the item b into the middle of array a, into a[p],
          moving the rest of the array over. Returns b.

      arrinsn:
        void arrinsn(T* a, int p, int n);
          Inserts n uninitialized items into array a starting at a[p],
          moving the rest of the array over.

      arraddnptr:
        T* arraddnptr(T* a, int n)
          Appends n uninitialized items onto array at the end.
          Returns a pointer to the first uninitialized item added.

      arraddnindex:
        size_t arraddnindex(T* a, int n)
          Appends n uninitialized items onto array at the end.
          Returns the index of the first uninitialized item added.

      arrdel:
        void arrdel(T* a, int p);
          Deletes the element at a[p], moving the rest of the array over.

      arrdeln:
        void arrdeln(T* a, int p, int n);
          Deletes n elements starting at a[p], moving the rest of the array over.

      arrdelswap:
        void arrdelswap(T* a, int p);
          Deletes the element at a[p], replacing it with the element from
          the end of the array. O(1) performance.

      arrsetlen:
        void arrsetlen(T* a, int n);
          Changes the length of the array to n. Allocates uninitialized
          slots at the end if necessary.

      arrsetcap:
        size_t arrsetcap(T* a, int n);
          Sets the length of allocated storage to at least n. It will not
          change the length of the array.

      arrcap:
        size_t arrcap(T* a);
          Returns the number of total elements the array can contain without
          needing to be reallocated.

  Hash maps & String hash maps

    Given T is a structure type: struct { TK key; TV value; }. Note that some
    functions do not require TV value and can have other fields. For string
    hash maps, TK must be 'char *'.

    Special interface:

      stbds_rand_seed:
        void stbds_rand_seed(size_t seed);
          For security against adversarially chosen data, you should seed the
          library with a strong random number. Or at least seed it with time().

      stbds_hash_string:
        size_t stbds_hash_string(char *str, size_t seed);
          Returns a hash value for a string.

      stbds_hash_bytes:
        size_t stbds_hash_bytes(void *p, size_t len, size_t seed);
          These functions hash an arbitrary number of bytes. The function
          uses a custom hash for 4- and 8-byte data, and a weakened version
          of SipHash for everything else. On 64-bit platforms you can get
          specification-compliant SipHash-2-4 on all data by defining
          STBDS_SIPHASH_2_4, at a significant cost in speed.

    Non-function interface:

      Declare an empty hash map of type T
        T* foo = NULL;

      Access the i'th entry in a hash table T* foo:
        foo[i]

    Function interface (actually macros):

      hmfree
      shfree
        void hmfree(T*);
        void shfree(T*);
          Frees the hashmap and sets the pointer to NULL.

      hmlen
      shlen
        ptrdiff_t hmlen(T*)
        ptrdiff_t shlen(T*)
          Returns the number of elements in the hashmap.

      hmlenu
      shlenu
        size_t hmlenu(T*)
        size_t shlenu(T*)
          Returns the number of elements in the hashmap.

      hmgeti
      shgeti
      hmgeti_ts
        ptrdiff_t hmgeti(T*, TK key)
        ptrdiff_t shgeti(T*, char* key)
        ptrdiff_t hmgeti_ts(T*, TK key, ptrdiff_t tempvar)
          Returns the index in the hashmap which has the key 'key', or -1
          if the key is not present.

      hmget
      hmget_ts
      shget
        TV hmget(T*, TK key)
        TV shget(T*, char* key)
        TV hmget_ts(T*, TK key, ptrdiff_t tempvar)
          Returns the value corresponding to 'key' in the hashmap.
          The structure must have a 'value' field

      hmgets
      shgets
        T hmgets(T*, TK key)
        T shgets(T*, char* key)
          Returns the structure corresponding to 'key' in the hashmap.

      hmgetp
      shgetp
      hmgetp_ts
      hmgetp_null
      shgetp_null
        T* hmgetp(T*, TK key)
        T* shgetp(T*, char* key)
        T* hmgetp_ts(T*, TK key, ptrdiff_t tempvar)
        T* hmgetp_null(T*, TK key)
        T* shgetp_null(T*, char *key)
          Returns a pointer to the structure corresponding to 'key' in
          the hashmap. Functions ending in "_null" return NULL if the key
          is not present in the hashmap; the others return a pointer to a
          structure holding the default value (but not the searched-for key).

      hmdefault
      shdefault
        TV hmdefault(T*, TV value)
        TV shdefault(T*, TV value)
          Sets the default value for the hashmap, the value which will be
          returned by hmget/shget if the key is not present.

      hmdefaults
      shdefaults
        TV hmdefaults(T*, T item)
        TV shdefaults(T*, T item)
          Sets the default struct for the hashmap, the contents which will be
          returned by hmgets/shgets if the key is not present.

      hmput
      shput
        TV hmput(T*, TK key, TV value)
        TV shput(T*, char* key, TV value)
          Inserts a <key,value> pair into the hashmap. If the key is already
          present in the hashmap, updates its value.

      hmputs
      shputs
        T hmputs(T*, T item)
        T shputs(T*, T item)
          Inserts a struct with T.key into the hashmap. If the struct is already
          present in the hashmap, updates it.

      hmdel
      shdel
        int hmdel(T*, TK key)
        int shdel(T*, char* key)
          If 'key' is in the hashmap, deletes its entry and returns 1.
          Otherwise returns 0.

    Function interface (actually macros) for strings only:

      sh_new_strdup
        void sh_new_strdup(T*);
          Overwrites the existing pointer with a newly allocated
          string hashmap which will automatically allocate and free
          each string key using realloc/free

      sh_new_arena
        void sh_new_arena(T*);
          Overwrites the existing pointer with a newly allocated
          string hashmap which will automatically allocate each string
          key to a string arena. Every string key ever used by this
          hash table remains in the arena until the arena is freed.
          Additionally, any key which is deleted and reinserted will
          be allocated multiple times in the string arena.

NOTES

  * These data structures are realloc'd when they grow, and the macro
    "functions" write to the provided pointer. This means: (a) the pointer
    must be an lvalue, and (b) the pointer to the data structure is not
    stable, and you must maintain it the same as you would a realloc'd
    pointer. For example, if you pass a pointer to a dynamic array to a
    function which updates it, the function must return back the new
    pointer to the caller. This is the price of trying to do this in C.

  * The following are the only functions that are thread-safe on a single data
    structure, i.e. can be run in multiple threads simultaneously on the same
    data structure
        hmlen        shlen
        hmlenu       shlenu
        hmget_ts     shget_ts
        hmgeti_ts    shgeti_ts
        hmgets_ts    shgets_ts

  * You iterate over the contents of a dynamic array and a hashmap in exactly
    the same way, using arrlen/hmlen/shlen:

      for (i=0; i < arrlen(foo); ++i)
         ... foo[i] ...

  * All operations except arrins/arrdel are O(1) amortized, but individual
    operations can be slow, so these data structures may not be suitable
    for real time use. Dynamic arrays double in capacity as needed, so
    elements are copied an average of once. Hash tables double/halve
    their size as needed, with appropriate hysteresis to maintain O(1)
    performance.

NOTES - DYNAMIC ARRAY

  * If you know how long a dynamic array is going to be in advance, you can avoid
    extra memory allocations by using arrsetlen to allocate it to that length in
    advance and use foo[n] while filling it out, or arrsetcap to allocate the memory
    for that length and use arrput/arrpush as normal.

  * Unlike some other versions of the dynamic array, this version should
    be safe to use with strict-aliasing optimizations.

NOTES - HASH MAP

  * For compilers other than GCC and clang (e.g. Visual Studio), for hmput/hmget/hmdel
    and variants, the key must be an lvalue (so the macro can take the address of it).
    Extensions are used that eliminate this requirement if you're using C99 and later
    in GCC or clang, or if you're using C++ in GCC. But note that this can make your
    code less portable.

  * To test for presence of a key in a hashmap, just do 'hmgeti(foo,key) >= 0'.

  * The iteration order of your data in the hashmap is determined solely by the
    order of insertions and deletions. In particular, if you never delete, new
    keys are always added at the end of the array. This will be consistent
    across all platforms and versions of the library. However, you should not
    attempt to serialize the internal hash table, as the hash is not consistent
    between different platforms, and may change with future versions of the library.

  * Use sh_new_arena() for string hashmaps that you never delete from. Initialize
    with NULL if you're managing the memory for your strings, or your strings are
    never freed (at least until the hashmap is freed). Otherwise, use sh_new_strdup().
    @TODO: make an arena variant that garbage collects the strings with a trivial
    copy collector into a new arena whenever the table shrinks / rebuilds. Since
    current arena recommendation is to only use arena if it never deletes, then
    this can just replace current arena implementation.

  * If adversarial input is a serious concern and you're on a 64-bit platform,
    enable STBDS_SIPHASH_2_4 (see the 'Compile-time options' section), and pass
    a strong random number to stbds_rand_seed.

  * The default value for the hash table is stored in foo[-1], so if you
    use code like 'hmget(T,k)->value = 5' you can accidentally overwrite
    the value stored by hmdefault if 'k' is not present.

CREDITS

  Sean Barrett -- library, idea for dynamic array API/implementation
  Per Vognsen  -- idea for hash table API/implementation
  Rafael Sachetto -- arrpop()
  github:HeroicKatora -- arraddn() reworking

  Bugfixes:
    Andy Durdin
    Shane Liesegang
    Vinh Truong
    Andreas Molzer
    github:hashitaku
    github:srdjanstipic
    Macoy Madson
    Andreas Vennstrom
    Tobias Mansfield-Williams
*/

#ifdef STBDS_UNIT_TESTS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef INCLUDE_STB_DS_H
#define INCLUDE_STB_DS_H

#include <stddef.h>
#include <string.h>

#ifndef STBDS_NO_SHORT_NAMES
#define arrlen      stbds_arrlen
#define arrlenu     stbds_arrlenu
#define arrput      stbds_arrput
#define arrpush     stbds_arrput
#define arrpop      stbds_arrpop
#define arrfree     stbds_arrfree
#define arraddn     stbds_arraddn // deprecated, use one of the following instead:
#define arraddnptr  stbds_arraddnptr
#define arraddnindex stbds_arraddnindex
#define arrsetlen   stbds_arrsetlen
#define arrlast     stbds_arrlast
#define arrins      stbds_arrins
#define arrinsn     stbds_arrinsn
#define arrdel      stbds_arrdel
#define arrdeln     stbds_arrdeln
#define arrdelswap  stbds_arrdelswap
#define arrcap      stbds_arrcap
#define arrsetcap   stbds_arrsetcap

#define hmput       stbds_hmput
#define hmputs      stbds_hmputs
#define hmget       stbds_hmget
#define hmget_ts    stbds_hmget_ts
#define hmgets      stbds_hmgets
#define hmgetp      stbds_hmgetp
#define hmgetp_ts   stbds_hmgetp_ts
#define hmgetp_null stbds_hmgetp_null
#define hmgeti      stbds_hmgeti
#define hmgeti_ts   stbds_hmgeti_ts
#define hmdel       stbds_hmdel
#define hmlen       stbds_hmlen
#define hmlenu      stbds_hmlenu
#define hmfree      stbds_hmfree
#define hmdefault   stbds_hmdefault
#define hmdefaults  stbds_hmdefaults

#define shput       stbds_shput
#define shputi      stbds_shputi
#define shputs      stbds_shputs
#define shget       stbds_shget
#define shgeti      stbds_shgeti
#define shgets      stbds_shgets
#define shgetp      stbds_shgetp
#define shgetp_null stbds_shgetp_null
#define shdel       stbds_shdel
#define shlen       stbds_shlen
#define shlenu      stbds_shlenu
#define shfree      stbds_shfree
#define shdefault   stbds_shdefault
#define shdefaults  stbds_shdefaults
#define sh_new_arena  stbds_sh_new_arena
#define sh_new_strdup stbds_sh_new_strdup

#define stralloc    stbds_stralloc
#define strreset    stbds_strreset
#endif

#if defined(STBDS_REALLOC) && !defined(STBDS_FREE) || !defined(STBDS_REALLOC) && defined(STBDS_FREE)
#error "You must define both STBDS_REALLOC and STBDS_FREE, or neither."
#endif
#if !defined(STBDS_REALLOC) && !defined(STBDS_FREE)
#include <stdlib.h>
#define STBDS_REALLOC(c,p,s) realloc(p,s)
#define STBDS_FREE(c,p)      free(p)
#endif

#ifdef _MSC_VER
#define STBDS_NOTUSED(v)  (void)(v)
#else
#define STBDS_NOTUSED(v)  (void)sizeof(v)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// for security against attackers, seed the library with a random number, at least time() but stronger is better
extern void stbds_rand_seed(size_t seed);

// these are the hash functions used internally if you want to test them or use them for other purposes
extern size_t stbds_hash_bytes(void *p, size_t len, size_t seed);
extern size_t stbds_hash_string(char *str, size_t seed);

// this is a simple string arena allocator, initialize with e.g. 'stbds_string_arena my_arena={0}'.
typedef struct stbds_string_arena stbds_string_arena;
extern char * stbds_stralloc(stbds_string_arena *a, char *str);
extern void   stbds_strreset(stbds_string_arena *a);

// have to #define STBDS_UNIT_TESTS to call this
extern void stbds_unit_tests(void);

///////////////
//
// Everything below here is implementation details
//

extern void * stbds_arrgrowf(void *a, size_t elemsize, size_t addlen, size_t min_cap);
extern void   stbds_arrfreef(void *a);
extern void   stbds_hmfree_func(void *p, size_t elemsize);
extern void * stbds_hmget_key(void *a, size_t elemsize, void *key, size_t keysize, int mode);
extern void * stbds_hmget_key_ts(void *a, size_t elemsize, void *key, size_t keysize, ptrdiff_t *temp, int mode);
extern void * stbds_hmput_default(void *a, size_t elemsize);
extern void * stbds_hmput_key(void *a, size_t elemsize, void *key, size_t keysize, int mode);
extern void * stbds_hmdel_key(void *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset, int mode);
extern void * stbds_shmode_func(size_t elemsize, int mode);

#ifdef __cplusplus
}
#endif

#if defined(__GNUC__) || defined(__clang__)
#define STBDS_HAS_TYPEOF
#ifdef __cplusplus
//#define STBDS_HAS_LITERAL_ARRAY  // this is currently broken for clang
#endif
#endif

#if !defined(__cplusplus)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define STBDS_HAS_LITERAL_ARRAY
#endif
#endif

// this macro takes the address of the argument, but on gcc/clang can accept rvalues
#if defined(STBDS_HAS_LITERAL_ARRAY) && defined(STBDS_HAS_TYPEOF)
  #if __clang__
  #define STBDS_ADDRESSOF(typevar, value)     ((__typeof__(typevar)[1]){value}) // literal array decays to pointer to value
  #else
  #define STBDS_ADDRESSOF(typevar, value)     ((typeof(typevar)[1]){value}) // literal array decays to pointer to value
  #endif
#else
#define STBDS_ADDRESSOF(typevar, value)     &(value)
#endif

#define STBDS_OFFSETOF(var,field)           ((char *) &(var)->field - (char *) (var))

#define stbds_header(t)  ((stbds_array_header *) (t) - 1)
#define stbds_temp(t)    stbds_header(t)->temp
#define stbds_temp_key(t) (*(char **) stbds_header(t)->hash_table)

#define stbds_arrsetcap(a,n)   (stbds_arrgrow(a,0,n))
#define stbds_arrsetlen(a,n)   ((stbds_arrcap(a) < (size_t) (n) ? stbds_arrsetcap((a),(size_t)(n)),0 : 0), (a) ? stbds_header(a)->length = (size_t) (n) : 0)
#define stbds_arrcap(a)        ((a) ? stbds_header(a)->capacity : 0)
#define stbds_arrlen(a)        ((a) ? (ptrdiff_t) stbds_header(a)->length : 0)
#define stbds_arrlenu(a)       ((a) ?             stbds_header(a)->length : 0)
#define stbds_arrput(a,v)      (stbds_arrmaybegrow(a,1), (a)[stbds_header(a)->length++] = (v))
#define stbds_arrpush          stbds_arrput  // synonym
#define stbds_arrpop(a)        (stbds_header(a)->length--, (a)[stbds_header(a)->length])
#define stbds_arraddn(a,n)     ((void)(stbds_arraddnindex(a, n)))    // deprecated, use one of the following instead:
#define stbds_arraddnptr(a,n)  (stbds_arrmaybegrow(a,n), (n) ? (stbds_header(a)->length += (n), &(a)[stbds_header(a)->length-(n)]) : (a))
#define stbds_arraddnindex(a,n)(stbds_arrmaybegrow(a,n), (n) ? (stbds_header(a)->length += (n), stbds_header(a)->length-(n)) : stbds_arrlen(a))
#define stbds_arraddnoff       stbds_arraddnindex
#define stbds_arrlast(a)       ((a)[stbds_header(a)->length-1])
#define stbds_arrfree(a)       ((void) ((a) ? STBDS_FREE(NULL,stbds_header(a)) : (void)0), (a)=NULL)
#define stbds_arrdel(a,i)      stbds_arrdeln(a,i,1)
#define stbds_arrdeln(a,i,n)   (memmove(&(a)[i], &(a)[(i)+(n)], sizeof *(a) * (stbds_header(a)->length-(n)-(i))), stbds_header(a)->length -= (n))
#define stbds_arrdelswap(a,i)  ((a)[i] = stbds_arrlast(a), stbds_header(a)->length -= 1)
#define stbds_arrinsn(a,i,n)   (stbds_arraddn((a),(n)), memmove(&(a)[(i)+(n)], &(a)[i], sizeof *(a) * (stbds_header(a)->length-(n)-(i))))
#define stbds_arrins(a,i,v)    (stbds_arrinsn((a),(i),1), (a)[i]=(v))

#define stbds_arrmaybegrow(a,n)  ((!(a) || stbds_header(a)->length + (n) > stbds_header(a)->capacity) \
                                  ? (stbds_arrgrow(a,n,0),0) : 0)

#define stbds_arrgrow(a,b,c)   ((a) = stbds_arrgrowf_wrapper((a), sizeof *(a), (b), (c)))

#define stbds_hmput(t, k, v) \
    ((t) = stbds_hmput_key_wrapper((t), sizeof *(t), (void*) STBDS_ADDRESSOF((t)->key, (k)), sizeof (t)->key, 0),   \
     (t)[stbds_temp((t)-1)].key = (k),    \
     (t)[stbds_temp((t)-1)].value = (v))

#define stbds_hmputs(t, s) \
    ((t) = stbds_hmput_key_wrapper((t), sizeof *(t), &(s).key, sizeof (s).key, STBDS_HM_BINARY), \
     (t)[stbds_temp((t)-1)] = (s))

#define stbds_hmgeti(t,k) \
    ((t) = stbds_hmget_key_wrapper((t), sizeof *(t), (void*) STBDS_ADDRESSOF((t)->key, (k)), sizeof (t)->key, STBDS_HM_BINARY), \
      stbds_temp((t)-1))

#define stbds_hmgeti_ts(t,k,temp) \
    ((t) = stbds_hmget_key_ts_wrapper((t), sizeof *(t), (void*) STBDS_ADDRESSOF((t)->key, (k)), sizeof (t)->key, &(temp), STBDS_HM_BINARY), \
      (temp))

#define stbds_hmgetp(t, k) \
    ((void) stbds_hmgeti(t,k), &(t)[stbds_temp((t)-1)])

#define stbds_hmgetp_ts(t, k, temp) \
    ((void) stbds_hmgeti_ts(t,k,temp), &(t)[temp])

#define stbds_hmdel(t,k) \
    (((t) = stbds_hmdel_key_wrapper((t),sizeof *(t), (void*) STBDS_ADDRESSOF((t)->key, (k)), sizeof (t)->key, STBDS_OFFSETOF((t),key), STBDS_HM_BINARY)),(t)?stbds_temp((t)-1):0)

#define stbds_hmdefault(t, v) \
    ((t) = stbds_hmput_default_wrapper((t), sizeof *(t)), (t)[-1].value = (v))

#define stbds_hmdefaults(t, s) \
    ((t) = stbds_hmput_default_wrapper((t), sizeof *(t)), (t)[-1] = (s))

#define stbds_hmfree(p)        \
    ((void) ((p) != NULL ? stbds_hmfree_func((p)-1,sizeof*(p)),0 : 0),(p)=NULL)

#define stbds_hmgets(t, k)    (*stbds_hmgetp(t,k))
#define stbds_hmget(t, k)     (stbds_hmgetp(t,k)->value)
#define stbds_hmget_ts(t, k, temp)  (stbds_hmgetp_ts(t,k,temp)->value)
#define stbds_hmlen(t)        ((t) ? (ptrdiff_t) stbds_header((t)-1)->length-1 : 0)
#define stbds_hmlenu(t)       ((t) ?             stbds_header((t)-1)->length-1 : 0)
#define stbds_hmgetp_null(t,k)  (stbds_hmgeti(t,k) == -1 ? NULL : &(t)[stbds_temp((t)-1)])

#define stbds_shput(t, k, v) \
    ((t) = stbds_hmput_key_wrapper((t), sizeof *(t), (void*) (k), sizeof (t)->key, STBDS_HM_STRING),   \
     (t)[stbds_temp((t)-1)].value = (v))

#define stbds_shputi(t, k, v) \
    ((t) = stbds_hmput_key_wrapper((t), sizeof *(t), (void*) (k), sizeof (t)->key, STBDS_HM_STRING),   \
     (t)[stbds_temp((t)-1)].value = (v), stbds_temp((t)-1))

#define stbds_shputs(t, s) \
    ((t) = stbds_hmput_key_wrapper((t), sizeof *(t), (void*) (s).key, sizeof (s).key, STBDS_HM_STRING), \
     (t)[stbds_temp((t)-1)] = (s), \
     (t)[stbds_temp((t)-1)].key = stbds_temp_key((t)-1)) // above line overwrites whole structure, so must rewrite key here if it was allocated internally

#define stbds_pshput(t, p) \
    ((t) = stbds_hmput_key_wrapper((t), sizeof *(t), (void*) (p)->key, sizeof (p)->key, STBDS_HM_PTR_TO_STRING), \
     (t)[stbds_temp((t)-1)] = (p))

#define stbds_shgeti(t,k) \
     ((t) = stbds_hmget_key_wrapper((t), sizeof *(t), (void*) (k), sizeof (t)->key, STBDS_HM_STRING), \
      stbds_temp((t)-1))

#define stbds_pshgeti(t,k) \
     ((t) = stbds_hmget_key_wrapper((t), sizeof *(t), (void*) (k), sizeof (*(t))->key, STBDS_HM_PTR_TO_STRING), \
      stbds_temp((t)-1))

#define stbds_shgetp(t, k) \
    ((void) stbds_shgeti(t,k), &(t)[stbds_temp((t)-1)])

#define stbds_pshget(t, k) \
    ((void) stbds_pshgeti(t,k), (t)[stbds_temp((t)-1)])

#define stbds_shdel(t,k) \
    (((t) = stbds_hmdel_key_wrapper((t),sizeof *(t), (void*) (k), sizeof (t)->key, STBDS_OFFSETOF((t),key), STBDS_HM_STRING)),(t)?stbds_temp((t)-1):0)
#define stbds_pshdel(t,k) \
    (((t) = stbds_hmdel_key_wrapper((t),sizeof *(t), (void*) (k), sizeof (*(t))->key, STBDS_OFFSETOF(*(t),key), STBDS_HM_PTR_TO_STRING)),(t)?stbds_temp((t)-1):0)

#define stbds_sh_new_arena(t)  \
    ((t) = stbds_shmode_func_wrapper(t, sizeof *(t), STBDS_SH_ARENA))
#define stbds_sh_new_strdup(t) \
    ((t) = stbds_shmode_func_wrapper(t, sizeof *(t), STBDS_SH_STRDUP))

#define stbds_shdefault(t, v)  stbds_hmdefault(t,v)
#define stbds_shdefaults(t, s) stbds_hmdefaults(t,s)

#define stbds_shfree       stbds_hmfree
#define stbds_shlenu       stbds_hmlenu

#define stbds_shgets(t, k) (*stbds_shgetp(t,k))
#define stbds_shget(t, k)  (stbds_shgetp(t,k)->value)
#define stbds_shgetp_null(t,k)  (stbds_shgeti(t,k) == -1 ? NULL : &(t)[stbds_temp((t)-1)])
#define stbds_shlen        stbds_hmlen

typedef struct
{
  size_t      length;
  size_t      capacity;
  void      * hash_table;
  ptrdiff_t   temp;
} stbds_array_header;

typedef struct stbds_string_block
{
  struct stbds_string_block *next;
  char storage[8];
} stbds_string_block;

struct stbds_string_arena
{
  stbds_string_block *storage;
  size_t remaining;
  unsigned char block;
  unsigned char mode;  // this isn't used by the string arena itself
};

#define STBDS_HM_BINARY         0
#define STBDS_HM_STRING         1

enum
{
   STBDS_SH_NONE,
   STBDS_SH_DEFAULT,
   STBDS_SH_STRDUP,
   STBDS_SH_ARENA
};

#ifdef __cplusplus
// in C we use implicit assignment from these void*-returning functions to T*.
// in C++ these templates make the same code work
template<class T> static T * stbds_arrgrowf_wrapper(T *a, size_t elemsize, size_t addlen, size_t min_cap) {
  return (T*)stbds_arrgrowf((void *)a, elemsize, addlen, min_cap);
}
template<class T> static T * stbds_hmget_key_wrapper(T *a, size_t elemsize, void *key, size_t keysize, int mode) {
  return (T*)stbds_hmget_key((void*)a, elemsize, key, keysize, mode);
}
template<class T> static T * stbds_hmget_key_ts_wrapper(T *a, size_t elemsize, void *key, size_t keysize, ptrdiff_t *temp, int mode) {
  return (T*)stbds_hmget_key_ts((void*)a, elemsize, key, keysize, temp, mode);
}
template<class T> static T * stbds_hmput_default_wrapper(T *a, size_t elemsize) {
  return (T*)stbds_hmput_default((void *)a, elemsize);
}
template<class T> static T * stbds_hmput_key_wrapper(T *a, size_t elemsize, void *key, size_t keysize, int mode) {
  return (T*)stbds_hmput_key((void*)a, elemsize, key, keysize, mode);
}
template<class T> static T * stbds_hmdel_key_wrapper(T *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset, int mode){
  return (T*)stbds_hmdel_key((void*)a, elemsize, key, keysize, keyoffset, mode);
}
template<class T> static T * stbds_shmode_func_wrapper(T *, size_t elemsize, int mode) {
  return (T*)stbds_shmode_func(elemsize, mode);
}
#else
#define stbds_arrgrowf_wrapper            stbds_arrgrowf
#define stbds_hmget_key_wrapper           stbds_hmget_key
#define stbds_hmget_key_ts_wrapper        stbds_hmget_key_ts
#define stbds_hmput_default_wrapper       stbds_hmput_default
#define stbds_hmput_key_wrapper           stbds_hmput_key
#define stbds_hmdel_key_wrapper           stbds_hmdel_key
#define stbds_shmode_func_wrapper(t,e,m)  stbds_shmode_func(e,m)
#endif

#endif // INCLUDE_STB_DS_H


//////////////////////////////////////////////////////////////////////////////
//
//   IMPLEMENTATION
//

#ifdef STB_DS_IMPLEMENTATION
#include <assert.h>
#include <string.h>

#ifndef STBDS_ASSERT
#define STBDS_ASSERT_WAS_UNDEFINED
#define STBDS_ASSERT(x)   ((void) 0)
#endif

#ifdef STBDS_STATISTICS
#define STBDS_STATS(x)   x
size_t stbds_array_grow;
size_t stbds_hash_grow;
size_t stbds_hash_shrink;
size_t stbds_hash_rebuild;
size_t stbds_hash_probes;
size_t stbds_hash_alloc;
size_t stbds_rehash_probes;
size_t stbds_rehash_items;
#else
#define STBDS_STATS(x)
#endif

//
// stbds_arr implementation
//

//int *prev_allocs[65536];
//int num_prev;

void *stbds_arrgrowf(void *a, size_t elemsize, size_t addlen, size_t min_cap)
{
  stbds_array_header temp={0}; // force debugging
  void *b;
  size_t min_len = stbds_arrlen(a) + addlen;
  (void) sizeof(temp);

  // compute the minimum capacity needed
  if (min_len > min_cap)
    min_cap = min_len;

  if (min_cap <= stbds_arrcap(a))
    return a;

  // increase needed capacity to guarantee O(1) amortized
  if (min_cap < 2 * stbds_arrcap(a))
    min_cap = 2 * stbds_arrcap(a);
  else if (min_cap < 4)
    min_cap = 4;

  //if (num_prev < 65536) if (a) prev_allocs[num_prev++] = (int *) ((char *) a+1);
  //if (num_prev == 2201)
  //  num_prev = num_prev;
  b = STBDS_REALLOC(NULL, (a) ? stbds_header(a) : 0, elemsize * min_cap + sizeof(stbds_array_header));
  //if (num_prev < 65536) prev_allocs[num_prev++] = (int *) (char *) b;
  b = (char *) b + sizeof(stbds_array_header);
  if (a == NULL) {
    stbds_header(b)->length = 0;
    stbds_header(b)->hash_table = 0;
    stbds_header(b)->temp = 0;
  } else {
    STBDS_STATS(++stbds_array_grow);
  }
  stbds_header(b)->capacity = min_cap;

  return b;
}

void stbds_arrfreef(void *a)
{
  STBDS_FREE(NULL, stbds_header(a));
}

//
// stbds_hm hash table implementation
//

#ifdef STBDS_INTERNAL_SMALL_BUCKET
#define STBDS_BUCKET_LENGTH      4
#else
#define STBDS_BUCKET_LENGTH      8
#endif

#define STBDS_BUCKET_SHIFT      (STBDS_BUCKET_LENGTH == 8 ? 3 : 2)
#define STBDS_BUCKET_MASK       (STBDS_BUCKET_LENGTH-1)
#define STBDS_CACHE_LINE_SIZE   64

#define STBDS_ALIGN_FWD(n,a)   (((n) + (a) - 1) & ~((a)-1))

typedef struct
{
   size_t    hash [STBDS_BUCKET_LENGTH];
   ptrdiff_t index[STBDS_BUCKET_LENGTH];
} stbds_hash_bucket; // in 32-bit, this is one 64-byte cache line; in 64-bit, each array is one 64-byte cache line

typedef struct
{
  char * temp_key; // this MUST be the first field of the hash table
  size_t slot_count;
  size_t used_count;
  size_t used_count_threshold;
  size_t used_count_shrink_threshold;
  size_t tombstone_count;
  size_t tombstone_count_threshold;
  size_t seed;
  size_t slot_count_log2;
  stbds_string_arena string;
  stbds_hash_bucket *storage; // not a separate allocation, just 64-byte aligned storage after this struct
} stbds_hash_index;

#define STBDS_INDEX_EMPTY    -1
#define STBDS_INDEX_DELETED  -2
#define STBDS_INDEX_IN_USE(x)  ((x) >= 0)

#define STBDS_HASH_EMPTY      0
#define STBDS_HASH_DELETED    1

static size_t stbds_hash_seed=0x31415926;

void stbds_rand_seed(size_t seed)
{
  stbds_hash_seed = seed;
}

#define stbds_load_32_or_64(var, temp, v32, v64_hi, v64_lo)                                          \
  temp = v64_lo ^ v32, temp <<= 16, temp <<= 16, temp >>= 16, temp >>= 16, /* discard if 32-bit */   \
  var = v64_hi, var <<= 16, var <<= 16,                                    /* discard if 32-bit */   \
  var ^= temp ^ v32

#define STBDS_SIZE_T_BITS           ((sizeof (size_t)) * 8)

static size_t stbds_probe_position(size_t hash, size_t slot_count, size_t slot_log2)
{
  size_t pos;
  STBDS_NOTUSED(slot_log2);
  pos = hash & (slot_count-1);
  #ifdef STBDS_INTERNAL_BUCKET_START
  pos &= ~STBDS_BUCKET_MASK;
  #endif
  return pos;
}

static size_t stbds_log2(size_t slot_count)
{
  size_t n=0;
  while (slot_count > 1) {
    slot_count >>= 1;
    ++n;
  }
  return n;
}

static stbds_hash_index *stbds_make_hash_index(size_t slot_count, stbds_hash_index *ot)
{
  stbds_hash_index *t;
  t = (stbds_hash_index *) STBDS_REALLOC(NULL,0,(slot_count >> STBDS_BUCKET_SHIFT) * sizeof(stbds_hash_bucket) + sizeof(stbds_hash_index) + STBDS_CACHE_LINE_SIZE-1);
  t->storage = (stbds_hash_bucket *) STBDS_ALIGN_FWD((size_t) (t+1), STBDS_CACHE_LINE_SIZE);
  t->slot_count = slot_count;
  t->slot_count_log2 = stbds_log2(slot_count);
  t->tombstone_count = 0;
  t->used_count = 0;

  #if 0 // A1
  t->used_count_threshold        = slot_count*12/16; // if 12/16th of table is occupied, grow
  t->tombstone_count_threshold   = slot_count* 2/16; // if tombstones are 2/16th of table, rebuild
  t->used_count_shrink_threshold = slot_count* 4/16; // if table is only 4/16th full, shrink
  #elif 1 // A2
  //t->used_count_threshold        = slot_count*12/16; // if 12/16th of table is occupied, grow
  //t->tombstone_count_threshold   = slot_count* 3/16; // if tombstones are 3/16th of table, rebuild
  //t->used_count_shrink_threshold = slot_count* 4/16; // if table is only 4/16th full, shrink

  // compute without overflowing
  t->used_count_threshold        = slot_count - (slot_count>>2);
  t->tombstone_count_threshold   = (slot_count>>3) + (slot_count>>4);
  t->used_count_shrink_threshold = slot_count >> 2;

  #elif 0 // B1
  t->used_count_threshold        = slot_count*13/16; // if 13/16th of table is occupied, grow
  t->tombstone_count_threshold   = slot_count* 2/16; // if tombstones are 2/16th of table, rebuild
  t->used_count_shrink_threshold = slot_count* 5/16; // if table is only 5/16th full, shrink
  #else // C1
  t->used_count_threshold        = slot_count*14/16; // if 14/16th of table is occupied, grow
  t->tombstone_count_threshold   = slot_count* 2/16; // if tombstones are 2/16th of table, rebuild
  t->used_count_shrink_threshold = slot_count* 6/16; // if table is only 6/16th full, shrink
  #endif
  // Following statistics were measured on a Core i7-6700 @ 4.00Ghz, compiled with clang 7.0.1 -O2
    // Note that the larger tables have high variance as they were run fewer times
  //     A1            A2          B1           C1
  //    0.10ms :     0.10ms :     0.10ms :     0.11ms :      2,000 inserts creating 2K table
  //    0.96ms :     0.95ms :     0.97ms :     1.04ms :     20,000 inserts creating 20K table
  //   14.48ms :    14.46ms :    10.63ms :    11.00ms :    200,000 inserts creating 200K table
  //  195.74ms :   196.35ms :   203.69ms :   214.92ms :  2,000,000 inserts creating 2M table
  // 2193.88ms :  2209.22ms :  2285.54ms :  2437.17ms : 20,000,000 inserts creating 20M table
  //   65.27ms :    53.77ms :    65.33ms :    65.47ms : 500,000 inserts & deletes in 2K table
  //   72.78ms :    62.45ms :    71.95ms :    72.85ms : 500,000 inserts & deletes in 20K table
  //   89.47ms :    77.72ms :    96.49ms :    96.75ms : 500,000 inserts & deletes in 200K table
  //   97.58ms :    98.14ms :    97.18ms :    97.53ms : 500,000 inserts & deletes in 2M table
  //  118.61ms :   119.62ms :   120.16ms :   118.86ms : 500,000 inserts & deletes in 20M table
  //  192.11ms :   194.39ms :   196.38ms :   195.73ms : 500,000 inserts & deletes in 200M table

  if (slot_count <= STBDS_BUCKET_LENGTH)
    t->used_count_shrink_threshold = 0;
  // to avoid infinite loop, we need to guarantee that at least one slot is empty and will terminate probes
  STBDS_ASSERT(t->used_count_threshold + t->tombstone_count_threshold < t->slot_count);
  STBDS_STATS(++stbds_hash_alloc);
  if (ot) {
    t->string = ot->string;
    // reuse old seed so we can reuse old hashes so below "copy out old data" doesn't do any hashing
    t->seed = ot->seed;
  } else {
    size_t a,b,temp;
    memset(&t->string, 0, sizeof(t->string));
    t->seed = stbds_hash_seed;
    // LCG
    // in 32-bit, a =          2147001325   b =  715136305
    // in 64-bit, a = 2862933555777941757   b = 3037000493
    stbds_load_32_or_64(a,temp, 2147001325, 0x27bb2ee6, 0x87b0b0fd);
    stbds_load_32_or_64(b,temp,  715136305,          0, 0xb504f32d);
    stbds_hash_seed = stbds_hash_seed  * a + b;
  }

  {
    size_t i,j;
    for (i=0; i < slot_count >> STBDS_BUCKET_SHIFT; ++i) {
      stbds_hash_bucket *b = &t->storage[i];
      for (j=0; j < STBDS_BUCKET_LENGTH; ++j)
        b->hash[j] = STBDS_HASH_EMPTY;
      for (j=0; j < STBDS_BUCKET_LENGTH; ++j)
        b->index[j] = STBDS_INDEX_EMPTY;
    }
  }

  // copy out the old data, if any
  if (ot) {
    size_t i,j;
    t->used_count = ot->used_count;
    for (i=0; i < ot->slot_count >> STBDS_BUCKET_SHIFT; ++i) {
      stbds_hash_bucket *ob = &ot->storage[i];
      for (j=0; j < STBDS_BUCKET_LENGTH; ++j) {
        if (STBDS_INDEX_IN_USE(ob->index[j])) {
          size_t hash = ob->hash[j];
          size_t pos = stbds_probe_position(hash, t->slot_count, t->slot_count_log2);
          size_t step = STBDS_BUCKET_LENGTH;
          STBDS_STATS(++stbds_rehash_items);
          for (;;) {
            size_t limit,z;
            stbds_hash_bucket *bucket;
            bucket = &t->storage[pos >> STBDS_BUCKET_SHIFT];
            STBDS_STATS(++stbds_rehash_probes);

            for (z=pos & STBDS_BUCKET_MASK; z < STBDS_BUCKET_LENGTH; ++z) {
              if (bucket->hash[z] == 0) {
                bucket->hash[z] = hash;
                bucket->index[z] = ob->index[j];
                goto done;
              }
            }

            limit = pos & STBDS_BUCKET_MASK;
            for (z = 0; z < limit; ++z) {
              if (bucket->hash[z] == 0) {
                bucket->hash[z] = hash;
                bucket->index[z] = ob->index[j];
                goto done;
              }
            }

            pos += step;                  // quadratic probing
            step += STBDS_BUCKET_LENGTH;
            pos &= (t->slot_count-1);
          }
        }
       done:
        ;
      }
    }
  }

  return t;
}

#define STBDS_ROTATE_LEFT(val, n)   (((val) << (n)) | ((val) >> (STBDS_SIZE_T_BITS - (n))))
#define STBDS_ROTATE_RIGHT(val, n)  (((val) >> (n)) | ((val) << (STBDS_SIZE_T_BITS - (n))))

size_t stbds_hash_string(char *str, size_t seed)
{
  size_t hash = seed;
  while (*str)
     hash = STBDS_ROTATE_LEFT(hash, 9) + (unsigned char) *str++;

  // Thomas Wang 64-to-32 bit mix function, hopefully also works in 32 bits
  hash ^= seed;
  hash = (~hash) + (hash << 18);
  hash ^= hash ^ STBDS_ROTATE_RIGHT(hash,31);
  hash = hash * 21;
  hash ^= hash ^ STBDS_ROTATE_RIGHT(hash,11);
  hash += (hash << 6);
  hash ^= STBDS_ROTATE_RIGHT(hash,22);
  return hash+seed;
}

#ifdef STBDS_SIPHASH_2_4
#define STBDS_SIPHASH_C_ROUNDS 2
#define STBDS_SIPHASH_D_ROUNDS 4
typedef int STBDS_SIPHASH_2_4_can_only_be_used_in_64_bit_builds[sizeof(size_t) == 8 ? 1 : -1];
#endif

#ifndef STBDS_SIPHASH_C_ROUNDS
#define STBDS_SIPHASH_C_ROUNDS 1
#endif
#ifndef STBDS_SIPHASH_D_ROUNDS
#define STBDS_SIPHASH_D_ROUNDS 1
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant, for do..while(0) and sizeof()==
#endif

static size_t stbds_siphash_bytes(void *p, size_t len, size_t seed)
{
  unsigned char *d = (unsigned char *) p;
  size_t i,j;
  size_t v0,v1,v2,v3, data;

  // hash that works on 32- or 64-bit registers without knowing which we have
  // (computes different results on 32-bit and 64-bit platform)
  // derived from siphash, but on 32-bit platforms very different as it uses 4 32-bit state not 4 64-bit
  v0 = ((((size_t) 0x736f6d65 << 16) << 16) + 0x70736575) ^  seed;
  v1 = ((((size_t) 0x646f7261 << 16) << 16) + 0x6e646f6d) ^ ~seed;
  v2 = ((((size_t) 0x6c796765 << 16) << 16) + 0x6e657261) ^  seed;
  v3 = ((((size_t) 0x74656462 << 16) << 16) + 0x79746573) ^ ~seed;

  #ifdef STBDS_TEST_SIPHASH_2_4
  // hardcoded with key material in the siphash test vectors
  v0 ^= 0x0706050403020100ull ^  seed;
  v1 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
  v2 ^= 0x0706050403020100ull ^  seed;
  v3 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
  #endif

  #define STBDS_SIPROUND() \
    do {                   \
      v0 += v1; v1 = STBDS_ROTATE_LEFT(v1, 13);  v1 ^= v0; v0 = STBDS_ROTATE_LEFT(v0,STBDS_SIZE_T_BITS/2); \
      v2 += v3; v3 = STBDS_ROTATE_LEFT(v3, 16);  v3 ^= v2;                                                 \
      v2 += v1; v1 = STBDS_ROTATE_LEFT(v1, 17);  v1 ^= v2; v2 = STBDS_ROTATE_LEFT(v2,STBDS_SIZE_T_BITS/2); \
      v0 += v3; v3 = STBDS_ROTATE_LEFT(v3, 21);  v3 ^= v0;                                                 \
    } while (0)

  for (i=0; i+sizeof(size_t) <= len; i += sizeof(size_t), d += sizeof(size_t)) {
    data = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
    data |= (size_t) (d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24)) << 16 << 16; // discarded if size_t == 4

    v3 ^= data;
    for (j=0; j < STBDS_SIPHASH_C_ROUNDS; ++j)
      STBDS_SIPROUND();
    v0 ^= data;
  }
  data = len << (STBDS_SIZE_T_BITS-8);
  switch (len - i) {
    case 7: data |= ((size_t) d[6] << 24) << 24; // fall through
    case 6: data |= ((size_t) d[5] << 20) << 20; // fall through
    case 5: data |= ((size_t) d[4] << 16) << 16; // fall through
    case 4: data |= (d[3] << 24); // fall through
    case 3: data |= (d[2] << 16); // fall through
    case 2: data |= (d[1] << 8); // fall through
    case 1: data |= d[0]; // fall through
    case 0: break;
  }
  v3 ^= data;
  for (j=0; j < STBDS_SIPHASH_C_ROUNDS; ++j)
    STBDS_SIPROUND();
  v0 ^= data;
  v2 ^= 0xff;
  for (j=0; j < STBDS_SIPHASH_D_ROUNDS; ++j)
    STBDS_SIPROUND();

#ifdef STBDS_SIPHASH_2_4
  return v0^v1^v2^v3;
#else
  return v1^v2^v3; // slightly stronger since v0^v3 in above cancels out final round operation? I tweeted at the authors of SipHash about this but they didn't reply
#endif
}

size_t stbds_hash_bytes(void *p, size_t len, size_t seed)
{
#ifdef STBDS_SIPHASH_2_4
  return stbds_siphash_bytes(p,len,seed);
#else
  unsigned char *d = (unsigned char *) p;

  if (len == 4) {
    unsigned int hash = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
    #if 0
    // HASH32-A  Bob Jenkin's hash function w/o large constants
    hash ^= seed;
    hash -= (hash<<6);
    hash ^= (hash>>17);
    hash -= (hash<<9);
    hash ^= seed;
    hash ^= (hash<<4);
    hash -= (hash<<3);
    hash ^= (hash<<10);
    hash ^= (hash>>15);
    #elif 1
    // HASH32-BB  Bob Jenkin's presumably-accidental version of Thomas Wang hash with rotates turned into shifts.
    // Note that converting these back to rotates makes it run a lot slower, presumably due to collisions, so I'm
    // not really sure what's going on.
    hash ^= seed;
    hash = (hash ^ 61) ^ (hash >> 16);
    hash = hash + (hash << 3);
    hash = hash ^ (hash >> 4);
    hash = hash * 0x27d4eb2d;
    hash ^= seed;
    hash = hash ^ (hash >> 15);
    #else  // HASH32-C   -  Murmur3
    hash ^= seed;
    hash *= 0xcc9e2d51;
    hash = (hash << 17) | (hash >> 15);
    hash *= 0x1b873593;
    hash ^= seed;
    hash = (hash << 19) | (hash >> 13);
    hash = hash*5 + 0xe6546b64;
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= seed;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    #endif
    // Following statistics were measured on a Core i7-6700 @ 4.00Ghz, compiled with clang 7.0.1 -O2
    // Note that the larger tables have high variance as they were run fewer times
    //  HASH32-A   //  HASH32-BB  //  HASH32-C
    //    0.10ms   //    0.10ms   //    0.10ms :      2,000 inserts creating 2K table
    //    0.96ms   //    0.95ms   //    0.99ms :     20,000 inserts creating 20K table
    //   14.69ms   //   14.43ms   //   14.97ms :    200,000 inserts creating 200K table
    //  199.99ms   //  195.36ms   //  202.05ms :  2,000,000 inserts creating 2M table
    // 2234.84ms   // 2187.74ms   // 2240.38ms : 20,000,000 inserts creating 20M table
    //   55.68ms   //   53.72ms   //   57.31ms : 500,000 inserts & deletes in 2K table
    //   63.43ms   //   61.99ms   //   65.73ms : 500,000 inserts & deletes in 20K table
    //   80.04ms   //   77.96ms   //   81.83ms : 500,000 inserts & deletes in 200K table
    //  100.42ms   //   97.40ms   //  102.39ms : 500,000 inserts & deletes in 2M table
    //  119.71ms   //  120.59ms   //  121.63ms : 500,000 inserts & deletes in 20M table
    //  185.28ms   //  195.15ms   //  187.74ms : 500,000 inserts & deletes in 200M table
    //   15.58ms   //   14.79ms   //   15.52ms : 200,000 inserts creating 200K table with varying key spacing

    return (((size_t) hash << 16 << 16) | hash) ^ seed;
  } else if (len == 8 && sizeof(size_t) == 8) {
    size_t hash = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
    hash |= (size_t) (d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24)) << 16 << 16; // avoid warning if size_t == 4
    hash ^= seed;
    hash = (~hash) + (hash << 21);
    hash ^= STBDS_ROTATE_RIGHT(hash,24);
    hash *= 265;
    hash ^= STBDS_ROTATE_RIGHT(hash,14);
    hash ^= seed;
    hash *= 21;
    hash ^= STBDS_ROTATE_RIGHT(hash,28);
    hash += (hash << 31);
    hash = (~hash) + (hash << 18);
    return hash;
  } else {
    return stbds_siphash_bytes(p,len,seed);
  }
#endif
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif


static int stbds_is_key_equal(void *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset, int mode, size_t i)
{
  if (mode >= STBDS_HM_STRING)
    return 0==strcmp((char *) key, * (char **) ((char *) a + elemsize*i + keyoffset));
  else
    return 0==memcmp(key, (char *) a + elemsize*i + keyoffset, keysize);
}

#define STBDS_HASH_TO_ARR(x,elemsize) ((char*) (x) - (elemsize))
#define STBDS_ARR_TO_HASH(x,elemsize) ((char*) (x) + (elemsize))

#define stbds_hash_table(a)  ((stbds_hash_index *) stbds_header(a)->hash_table)

void stbds_hmfree_func(void *a, size_t elemsize)
{
  if (a == NULL) return;
  if (stbds_hash_table(a) != NULL) {
    if (stbds_hash_table(a)->string.mode == STBDS_SH_STRDUP) {
      size_t i;
      // skip 0th element, which is default
      for (i=1; i < stbds_header(a)->length; ++i)
        STBDS_FREE(NULL, *(char**) ((char *) a + elemsize*i));
    }
    stbds_strreset(&stbds_hash_table(a)->string);
  }
  STBDS_FREE(NULL, stbds_header(a)->hash_table);
  STBDS_FREE(NULL, stbds_header(a));
}

static ptrdiff_t stbds_hm_find_slot(void *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset, int mode)
{
  void *raw_a = STBDS_HASH_TO_ARR(a,elemsize);
  stbds_hash_index *table = stbds_hash_table(raw_a);
  size_t hash = mode >= STBDS_HM_STRING ? stbds_hash_string((char*)key,table->seed) : stbds_hash_bytes(key, keysize,table->seed);
  size_t step = STBDS_BUCKET_LENGTH;
  size_t limit,i;
  size_t pos;
  stbds_hash_bucket *bucket;

  if (hash < 2) hash += 2; // stored hash values are forbidden from being 0, so we can detect empty slots

  pos = stbds_probe_position(hash, table->slot_count, table->slot_count_log2);

  for (;;) {
    STBDS_STATS(++stbds_hash_probes);
    bucket = &table->storage[pos >> STBDS_BUCKET_SHIFT];

    // start searching from pos to end of bucket, this should help performance on small hash tables that fit in cache
    for (i=pos & STBDS_BUCKET_MASK; i < STBDS_BUCKET_LENGTH; ++i) {
      if (bucket->hash[i] == hash) {
        if (stbds_is_key_equal(a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
          return (pos & ~STBDS_BUCKET_MASK)+i;
        }
      } else if (bucket->hash[i] == STBDS_HASH_EMPTY) {
        return -1;
      }
    }

    // search from beginning of bucket to pos
    limit = pos & STBDS_BUCKET_MASK;
    for (i = 0; i < limit; ++i) {
      if (bucket->hash[i] == hash) {
        if (stbds_is_key_equal(a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
          return (pos & ~STBDS_BUCKET_MASK)+i;
        }
      } else if (bucket->hash[i] == STBDS_HASH_EMPTY) {
        return -1;
      }
    }

    // quadratic probing
    pos += step;
    step += STBDS_BUCKET_LENGTH;
    pos &= (table->slot_count-1);
  }
  /* NOTREACHED */
}

void * stbds_hmget_key_ts(void *a, size_t elemsize, void *key, size_t keysize, ptrdiff_t *temp, int mode)
{
  size_t keyoffset = 0;
  if (a == NULL) {
    // make it non-empty so we can return a temp
    a = stbds_arrgrowf(0, elemsize, 0, 1);
    stbds_header(a)->length += 1;
    memset(a, 0, elemsize);
    *temp = STBDS_INDEX_EMPTY;
    // adjust a to point after the default element
    return STBDS_ARR_TO_HASH(a,elemsize);
  } else {
    stbds_hash_index *table;
    void *raw_a = STBDS_HASH_TO_ARR(a,elemsize);
    // adjust a to point to the default element
    table = (stbds_hash_index *) stbds_header(raw_a)->hash_table;
    if (table == 0) {
      *temp = -1;
    } else {
      ptrdiff_t slot = stbds_hm_find_slot(a, elemsize, key, keysize, keyoffset, mode);
      if (slot < 0) {
        *temp = STBDS_INDEX_EMPTY;
      } else {
        stbds_hash_bucket *b = &table->storage[slot >> STBDS_BUCKET_SHIFT];
        *temp = b->index[slot & STBDS_BUCKET_MASK];
      }
    }
    return a;
  }
}

void * stbds_hmget_key(void *a, size_t elemsize, void *key, size_t keysize, int mode)
{
  ptrdiff_t temp;
  void *p = stbds_hmget_key_ts(a, elemsize, key, keysize, &temp, mode);
  stbds_temp(STBDS_HASH_TO_ARR(p,elemsize)) = temp;
  return p;
}

void * stbds_hmput_default(void *a, size_t elemsize)
{
  // three cases:
  //   a is NULL <- allocate
  //   a has a hash table but no entries, because of shmode <- grow
  //   a has entries <- do nothing
  if (a == NULL || stbds_header(STBDS_HASH_TO_ARR(a,elemsize))->length == 0) {
    a = stbds_arrgrowf(a ? STBDS_HASH_TO_ARR(a,elemsize) : NULL, elemsize, 0, 1);
    stbds_header(a)->length += 1;
    memset(a, 0, elemsize);
    a=STBDS_ARR_TO_HASH(a,elemsize);
  }
  return a;
}

static char *stbds_strdup(char *str);

void *stbds_hmput_key(void *a, size_t elemsize, void *key, size_t keysize, int mode)
{
  size_t keyoffset=0;
  void *raw_a;
  stbds_hash_index *table;

  if (a == NULL) {
    a = stbds_arrgrowf(0, elemsize, 0, 1);
    memset(a, 0, elemsize);
    stbds_header(a)->length += 1;
    // adjust a to point AFTER the default element
    a = STBDS_ARR_TO_HASH(a,elemsize);
  }

  // adjust a to point to the default element
  raw_a = a;
  a = STBDS_HASH_TO_ARR(a,elemsize);

  table = (stbds_hash_index *) stbds_header(a)->hash_table;

  if (table == NULL || table->used_count >= table->used_count_threshold) {
    stbds_hash_index *nt;
    size_t slot_count;

    slot_count = (table == NULL) ? STBDS_BUCKET_LENGTH : table->slot_count*2;
    nt = stbds_make_hash_index(slot_count, table);
    if (table)
      STBDS_FREE(NULL, table);
    else
      nt->string.mode = mode >= STBDS_HM_STRING ? STBDS_SH_DEFAULT : 0;
    stbds_header(a)->hash_table = table = nt;
    STBDS_STATS(++stbds_hash_grow);
  }

  // we iterate hash table explicitly because we want to track if we saw a tombstone
  {
    size_t hash = mode >= STBDS_HM_STRING ? stbds_hash_string((char*)key,table->seed) : stbds_hash_bytes(key, keysize,table->seed);
    size_t step = STBDS_BUCKET_LENGTH;
    size_t pos;
    ptrdiff_t tombstone = -1;
    stbds_hash_bucket *bucket;

    // stored hash values are forbidden from being 0, so we can detect empty slots to early out quickly
    if (hash < 2) hash += 2;

    pos = stbds_probe_position(hash, table->slot_count, table->slot_count_log2);

    for (;;) {
      size_t limit, i;
      STBDS_STATS(++stbds_hash_probes);
      bucket = &table->storage[pos >> STBDS_BUCKET_SHIFT];

      // start searching from pos to end of bucket
      for (i=pos & STBDS_BUCKET_MASK; i < STBDS_BUCKET_LENGTH; ++i) {
        if (bucket->hash[i] == hash) {
          if (stbds_is_key_equal(raw_a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
            stbds_temp(a) = bucket->index[i];
            if (mode >= STBDS_HM_STRING)
              stbds_temp_key(a) = * (char **) ((char *) raw_a + elemsize*bucket->index[i] + keyoffset);
            return STBDS_ARR_TO_HASH(a,elemsize);
          }
        } else if (bucket->hash[i] == 0) {
          pos = (pos & ~STBDS_BUCKET_MASK) + i;
          goto found_empty_slot;
        } else if (tombstone < 0) {
          if (bucket->index[i] == STBDS_INDEX_DELETED)
            tombstone = (ptrdiff_t) ((pos & ~STBDS_BUCKET_MASK) + i);
        }
      }

      // search from beginning of bucket to pos
      limit = pos & STBDS_BUCKET_MASK;
      for (i = 0; i < limit; ++i) {
        if (bucket->hash[i] == hash) {
          if (stbds_is_key_equal(raw_a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
            stbds_temp(a) = bucket->index[i];
            return STBDS_ARR_TO_HASH(a,elemsize);
          }
        } else if (bucket->hash[i] == 0) {
          pos = (pos & ~STBDS_BUCKET_MASK) + i;
          goto found_empty_slot;
        } else if (tombstone < 0) {
          if (bucket->index[i] == STBDS_INDEX_DELETED)
            tombstone = (ptrdiff_t) ((pos & ~STBDS_BUCKET_MASK) + i);
        }
      }

      // quadratic probing
      pos += step;
      step += STBDS_BUCKET_LENGTH;
      pos &= (table->slot_count-1);
    }
   found_empty_slot:
    if (tombstone >= 0) {
      pos = tombstone;
      --table->tombstone_count;
    }
    ++table->used_count;

    {
      ptrdiff_t i = (ptrdiff_t) stbds_arrlen(a);
      // we want to do stbds_arraddn(1), but we can't use the macros since we don't have something of the right type
      if ((size_t) i+1 > stbds_arrcap(a))
        *(void **) &a = stbds_arrgrowf(a, elemsize, 1, 0);
      raw_a = STBDS_ARR_TO_HASH(a,elemsize);

      STBDS_ASSERT((size_t) i+1 <= stbds_arrcap(a));
      stbds_header(a)->length = i+1;
      bucket = &table->storage[pos >> STBDS_BUCKET_SHIFT];
      bucket->hash[pos & STBDS_BUCKET_MASK] = hash;
      bucket->index[pos & STBDS_BUCKET_MASK] = i-1;
      stbds_temp(a) = i-1;

      switch (table->string.mode) {
         case STBDS_SH_STRDUP:  stbds_temp_key(a) = *(char **) ((char *) a + elemsize*i) = stbds_strdup((char*) key); break;
         case STBDS_SH_ARENA:   stbds_temp_key(a) = *(char **) ((char *) a + elemsize*i) = stbds_stralloc(&table->string, (char*)key); break;
         case STBDS_SH_DEFAULT: stbds_temp_key(a) = *(char **) ((char *) a + elemsize*i) = (char *) key; break;
         default:                memcpy((char *) a + elemsize*i, key, keysize); break;
      }
    }
    return STBDS_ARR_TO_HASH(a,elemsize);
  }
}

void * stbds_shmode_func(size_t elemsize, int mode)
{
  void *a = stbds_arrgrowf(0, elemsize, 0, 1);
  stbds_hash_index *h;
  memset(a, 0, elemsize);
  stbds_header(a)->length = 1;
  stbds_header(a)->hash_table = h = (stbds_hash_index *) stbds_make_hash_index(STBDS_BUCKET_LENGTH, NULL);
  h->string.mode = (unsigned char) mode;
  return STBDS_ARR_TO_HASH(a,elemsize);
}

void * stbds_hmdel_key(void *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset, int mode)
{
  if (a == NULL) {
    return 0;
  } else {
    stbds_hash_index *table;
    void *raw_a = STBDS_HASH_TO_ARR(a,elemsize);
    table = (stbds_hash_index *) stbds_header(raw_a)->hash_table;
    stbds_temp(raw_a) = 0;
    if (table == 0) {
      return a;
    } else {
      ptrdiff_t slot;
      slot = stbds_hm_find_slot(a, elemsize, key, keysize, keyoffset, mode);
      if (slot < 0)
        return a;
      else {
        stbds_hash_bucket *b = &table->storage[slot >> STBDS_BUCKET_SHIFT];
        int i = slot & STBDS_BUCKET_MASK;
        ptrdiff_t old_index = b->index[i];
        ptrdiff_t final_index = (ptrdiff_t) stbds_arrlen(raw_a)-1-1; // minus one for the raw_a vs a, and minus one for 'last'
        STBDS_ASSERT(slot < (ptrdiff_t) table->slot_count);
        --table->used_count;
        ++table->tombstone_count;
        stbds_temp(raw_a) = 1;
        STBDS_ASSERT(table->used_count >= 0);
        //STBDS_ASSERT(table->tombstone_count < table->slot_count/4);
        b->hash[i] = STBDS_HASH_DELETED;
        b->index[i] = STBDS_INDEX_DELETED;

        if (mode == STBDS_HM_STRING && table->string.mode == STBDS_SH_STRDUP)
          STBDS_FREE(NULL, *(char**) ((char *) a+elemsize*old_index));

        // if indices are the same, memcpy is a no-op, but back-pointer-fixup will fail, so skip
        if (old_index != final_index) {
          // swap delete
          memmove((char*) a + elemsize*old_index, (char*) a + elemsize*final_index, elemsize);

          // now find the slot for the last element
          if (mode == STBDS_HM_STRING)
            slot = stbds_hm_find_slot(a, elemsize, *(char**) ((char *) a+elemsize*old_index + keyoffset), keysize, keyoffset, mode);
          else
            slot = stbds_hm_find_slot(a, elemsize,  (char* ) a+elemsize*old_index + keyoffset, keysize, keyoffset, mode);
          STBDS_ASSERT(slot >= 0);
          b = &table->storage[slot >> STBDS_BUCKET_SHIFT];
          i = slot & STBDS_BUCKET_MASK;
          STBDS_ASSERT(b->index[i] == final_index);
          b->index[i] = old_index;
        }
        stbds_header(raw_a)->length -= 1;

        if (table->used_count < table->used_count_shrink_threshold && table->slot_count > STBDS_BUCKET_LENGTH) {
          stbds_header(raw_a)->hash_table = stbds_make_hash_index(table->slot_count>>1, table);
          STBDS_FREE(NULL, table);
          STBDS_STATS(++stbds_hash_shrink);
        } else if (table->tombstone_count > table->tombstone_count_threshold) {
          stbds_header(raw_a)->hash_table = stbds_make_hash_index(table->slot_count   , table);
          STBDS_FREE(NULL, table);
          STBDS_STATS(++stbds_hash_rebuild);
        }

        return a;
      }
    }
  }
  /* NOTREACHED */
}

static char *stbds_strdup(char *str)
{
  // to keep replaceable allocator simple, we don't want to use strdup.
  // rolling our own also avoids problem of strdup vs _strdup
  size_t len = strlen(str)+1;
  char *p = (char*) STBDS_REALLOC(NULL, 0, len);
  memmove(p, str, len);
  return p;
}

#ifndef STBDS_STRING_ARENA_BLOCKSIZE_MIN
#define STBDS_STRING_ARENA_BLOCKSIZE_MIN  512u
#endif
#ifndef STBDS_STRING_ARENA_BLOCKSIZE_MAX
#define STBDS_STRING_ARENA_BLOCKSIZE_MAX  (1u<<20)
#endif

char *stbds_stralloc(stbds_string_arena *a, char *str)
{
  char *p;
  size_t len = strlen(str)+1;
  if (len > a->remaining) {
    // compute the next blocksize
    size_t blocksize = a->block;

    // size is 512, 512, 1024, 1024, 2048, 2048, 4096, 4096, etc., so that
    // there are log(SIZE) allocations to free when we destroy the table
    blocksize = (size_t) (STBDS_STRING_ARENA_BLOCKSIZE_MIN) << (blocksize>>1);

    // if size is under 1M, advance to next blocktype
    if (blocksize < (size_t)(STBDS_STRING_ARENA_BLOCKSIZE_MAX))
      ++a->block;

    if (len > blocksize) {
      // if string is larger than blocksize, then just allocate the full size.
      // note that we still advance string_block so block size will continue
      // increasing, so e.g. if somebody only calls this with 1000-long strings,
      // eventually the arena will start doubling and handling those as well
      stbds_string_block *sb = (stbds_string_block *) STBDS_REALLOC(NULL, 0, sizeof(*sb)-8 + len);
      memmove(sb->storage, str, len);
      if (a->storage) {
        // insert it after the first element, so that we don't waste the space there
        sb->next = a->storage->next;
        a->storage->next = sb;
      } else {
        sb->next = 0;
        a->storage = sb;
        a->remaining = 0; // this is redundant, but good for clarity
      }
      return sb->storage;
    } else {
      stbds_string_block *sb = (stbds_string_block *) STBDS_REALLOC(NULL, 0, sizeof(*sb)-8 + blocksize);
      sb->next = a->storage;
      a->storage = sb;
      a->remaining = blocksize;
    }
  }

  STBDS_ASSERT(len <= a->remaining);
  p = a->storage->storage + a->remaining - len;
  a->remaining -= len;
  memmove(p, str, len);
  return p;
}

void stbds_strreset(stbds_string_arena *a)
{
  stbds_string_block *x,*y;
  x = a->storage;
  while (x) {
    y = x->next;
    STBDS_FREE(NULL, x);
    x = y;
  }
  memset(a, 0, sizeof(*a));
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//   UNIT TESTS
//

#ifdef STBDS_UNIT_TESTS
#include <stdio.h>
#ifdef STBDS_ASSERT_WAS_UNDEFINED
#undef STBDS_ASSERT
#endif
#ifndef STBDS_ASSERT
#define STBDS_ASSERT assert
#include <assert.h>
#endif

typedef struct { int key,b,c,d; } stbds_struct;
typedef struct { int key[2],b,c,d; } stbds_struct2;

static char buffer[256];
char *strkey(int n)
{
#if defined(_WIN32) && defined(__STDC_WANT_SECURE_LIB__)
   sprintf_s(buffer, sizeof(buffer), "test_%d", n);
#else
   sprintf(buffer, "test_%d", n);
#endif
   return buffer;
}

void stbds_unit_tests(void)
{
#if defined(_MSC_VER) && _MSC_VER <= 1200 && defined(__cplusplus)
  // VC6 C++ doesn't like the template<> trick on unnamed structures, so do nothing!
  STBDS_ASSERT(0);
#else
  const int testsize = 100000;
  const int testsize2 = testsize/20;
  int *arr=NULL;
  struct { int   key;        int value; }  *intmap  = NULL;
  struct { char *key;        int value; }  *strmap  = NULL, s;
  struct { stbds_struct key; int value; }  *map     = NULL;
  stbds_struct                             *map2    = NULL;
  stbds_struct2                            *map3    = NULL;
  stbds_string_arena                        sa      = { 0 };
  int key3[2] = { 1,2 };
  ptrdiff_t temp;

  int i,j;

  STBDS_ASSERT(arrlen(arr)==0);
  for (i=0; i < 20000; i += 50) {
    for (j=0; j < i; ++j)
      arrpush(arr,j);
    arrfree(arr);
  }

  for (i=0; i < 4; ++i) {
    arrpush(arr,1); arrpush(arr,2); arrpush(arr,3); arrpush(arr,4);
    arrdel(arr,i);
    arrfree(arr);
    arrpush(arr,1); arrpush(arr,2); arrpush(arr,3); arrpush(arr,4);
    arrdelswap(arr,i);
    arrfree(arr);
  }

  for (i=0; i < 5; ++i) {
    arrpush(arr,1); arrpush(arr,2); arrpush(arr,3); arrpush(arr,4);
    stbds_arrins(arr,i,5);
    STBDS_ASSERT(arr[i] == 5);
    if (i < 4)
      STBDS_ASSERT(arr[4] == 4);
    arrfree(arr);
  }

  i = 1;
  STBDS_ASSERT(hmgeti(intmap,i) == -1);
  hmdefault(intmap, -2);
  STBDS_ASSERT(hmgeti(intmap, i) == -1);
  STBDS_ASSERT(hmget (intmap, i) == -2);
  for (i=0; i < testsize; i+=2)
    hmput(intmap, i, i*5);
  for (i=0; i < testsize; i+=1) {
    if (i & 1) STBDS_ASSERT(hmget(intmap, i) == -2 );
    else       STBDS_ASSERT(hmget(intmap, i) == i*5);
    if (i & 1) STBDS_ASSERT(hmget_ts(intmap, i, temp) == -2 );
    else       STBDS_ASSERT(hmget_ts(intmap, i, temp) == i*5);
  }
  for (i=0; i < testsize; i+=2)
    hmput(intmap, i, i*3);
  for (i=0; i < testsize; i+=1)
    if (i & 1) STBDS_ASSERT(hmget(intmap, i) == -2 );
    else       STBDS_ASSERT(hmget(intmap, i) == i*3);
  for (i=2; i < testsize; i+=4)
    hmdel(intmap, i); // delete half the entries
  for (i=0; i < testsize; i+=1)
    if (i & 3) STBDS_ASSERT(hmget(intmap, i) == -2 );
    else       STBDS_ASSERT(hmget(intmap, i) == i*3);
  for (i=0; i < testsize; i+=1)
    hmdel(intmap, i); // delete the rest of the entries
  for (i=0; i < testsize; i+=1)
    STBDS_ASSERT(hmget(intmap, i) == -2 );
  hmfree(intmap);
  for (i=0; i < testsize; i+=2)
    hmput(intmap, i, i*3);
  hmfree(intmap);

  #if defined(__clang__) || defined(__GNUC__)
  #ifndef __cplusplus
  intmap = NULL;
  hmput(intmap, 15, 7);
  hmput(intmap, 11, 3);
  hmput(intmap,  9, 5);
  STBDS_ASSERT(hmget(intmap, 9) == 5);
  STBDS_ASSERT(hmget(intmap, 11) == 3);
  STBDS_ASSERT(hmget(intmap, 15) == 7);
  #endif
  #endif

  for (i=0; i < testsize; ++i)
    stralloc(&sa, strkey(i));
  strreset(&sa);

  {
    s.key = "a", s.value = 1;
    shputs(strmap, s);
    STBDS_ASSERT(*strmap[0].key == 'a');
    STBDS_ASSERT(strmap[0].key == s.key);
    STBDS_ASSERT(strmap[0].value == s.value);
    shfree(strmap);
  }

  {
    s.key = "a", s.value = 1;
    sh_new_strdup(strmap);
    shputs(strmap, s);
    STBDS_ASSERT(*strmap[0].key == 'a');
    STBDS_ASSERT(strmap[0].key != s.key);
    STBDS_ASSERT(strmap[0].value == s.value);
    shfree(strmap);
  }

  {
    s.key = "a", s.value = 1;
    sh_new_arena(strmap);
    shputs(strmap, s);
    STBDS_ASSERT(*strmap[0].key == 'a');
    STBDS_ASSERT(strmap[0].key != s.key);
    STBDS_ASSERT(strmap[0].value == s.value);
    shfree(strmap);
  }

  for (j=0; j < 2; ++j) {
    STBDS_ASSERT(shgeti(strmap,"foo") == -1);
    if (j == 0)
      sh_new_strdup(strmap);
    else
      sh_new_arena(strmap);
    STBDS_ASSERT(shgeti(strmap,"foo") == -1);
    shdefault(strmap, -2);
    STBDS_ASSERT(shgeti(strmap,"foo") == -1);
    for (i=0; i < testsize; i+=2)
      shput(strmap, strkey(i), i*3);
    for (i=0; i < testsize; i+=1)
      if (i & 1) STBDS_ASSERT(shget(strmap, strkey(i)) == -2 );
      else       STBDS_ASSERT(shget(strmap, strkey(i)) == i*3);
    for (i=2; i < testsize; i+=4)
      shdel(strmap, strkey(i)); // delete half the entries
    for (i=0; i < testsize; i+=1)
      if (i & 3) STBDS_ASSERT(shget(strmap, strkey(i)) == -2 );
      else       STBDS_ASSERT(shget(strmap, strkey(i)) == i*3);
    for (i=0; i < testsize; i+=1)
      shdel(strmap, strkey(i)); // delete the rest of the entries
    for (i=0; i < testsize; i+=1)
      STBDS_ASSERT(shget(strmap, strkey(i)) == -2 );
    shfree(strmap);
  }

  {
    struct { char *key; char value; } *hash = NULL;
    char name[4] = "jen";
    shput(hash, "bob"   , 'h');
    shput(hash, "sally" , 'e');
    shput(hash, "fred"  , 'l');
    shput(hash, "jen"   , 'x');
    shput(hash, "doug"  , 'o');

    shput(hash, name    , 'l');
    shfree(hash);
  }

  for (i=0; i < testsize; i += 2) {
    stbds_struct s = { i,i*2,i*3,i*4 };
    hmput(map, s, i*5);
  }

  for (i=0; i < testsize; i += 1) {
    stbds_struct s = { i,i*2,i*3  ,i*4 };
    stbds_struct t = { i,i*2,i*3+1,i*4 };
    if (i & 1) STBDS_ASSERT(hmget(map, s) == 0);
    else       STBDS_ASSERT(hmget(map, s) == i*5);
    if (i & 1) STBDS_ASSERT(hmget_ts(map, s, temp) == 0);
    else       STBDS_ASSERT(hmget_ts(map, s, temp) == i*5);
    //STBDS_ASSERT(hmget(map, t.key) == 0);
  }

  for (i=0; i < testsize; i += 2) {
    stbds_struct s = { i,i*2,i*3,i*4 };
    hmputs(map2, s);
  }
  hmfree(map);

  for (i=0; i < testsize; i += 1) {
    stbds_struct s = { i,i*2,i*3,i*4 };
    stbds_struct t = { i,i*2,i*3+1,i*4 };
    if (i & 1) STBDS_ASSERT(hmgets(map2, s.key).d == 0);
    else       STBDS_ASSERT(hmgets(map2, s.key).d == i*4);
    //STBDS_ASSERT(hmgetp(map2, t.key) == 0);
  }
  hmfree(map2);

  for (i=0; i < testsize; i += 2) {
    stbds_struct2 s = { { i,i*2 }, i*3,i*4, i*5 };
    hmputs(map3, s);
  }
  for (i=0; i < testsize; i += 1) {
    stbds_struct2 s = { { i,i*2}, i*3, i*4, i*5 };
    stbds_struct2 t = { { i,i*2}, i*3+1, i*4, i*5 };
    if (i & 1) STBDS_ASSERT(hmgets(map3, s.key).d == 0);
    else       STBDS_ASSERT(hmgets(map3, s.key).d == i*5);
    //STBDS_ASSERT(hmgetp(map3, t.key) == 0);
  }
#endif
}
#endif


/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2019 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/

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

#ifdef __cplusplus
}
#endif

#endif // STB_STB_STB_H_INCLUDED

#ifdef STB_STB_STB_IMPLEMENTATION

#include "lib/c.h"
#include <ctype.h>

/// @brief Read THEN input char (wint_t)
#define READ(C) *(C)++
/// @brief Run another rule on current input.
#define INCL(LABEL) goto LABEL
/// @brief Read next character, and run anothe rule.
#define THEN(LABEL, C) \
    do {               \
        C++;       \
        INCL(LABEL);   \
    } while (0)

slice_t extract_include(char const *const line) {
    slice_t filename = { .val = line };

    char const *c = line;

start:
    switch (READ(c)) {
    case '#':
        if (READ(c) == 'i'
            && READ(c) == 'n'
            && READ(c) == 'c'
            && READ(c) == 'l'
            && READ(c) == 'u'
            && READ(c) == 'd'
            && READ(c) == 'e') {
            THEN(start_path, c);
        }
        return (slice_t) { 0 };
    default:
        if (*c && isspace(*c)) goto start;
        return (slice_t) { 0 };
    }

start_path:
    switch (READ(c)) {
    case '<': THEN(usr, c);
    case '"': THEN(sys, c);
    default:
        if (*c && isspace(*c)) goto start_path;
        return (slice_t) { 0 };
    }

usr:
    switch (READ(c)) {
    case '"': return filename;
    case '\n':
    case '\0': return (slice_t) { 0 };
    default:
        filename.len++;
        goto usr;
    }

sys:
    switch (READ(c)) {
    case '>': return filename;
    case '\n':
    case '\0': return (slice_t) { 0 };
    default:
        filename.len++;
        goto sys;
    }
}

#include "lib/library_order.h"
#include "lib/c.h"
#include "lib/stb_ds.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

typedef struct {
    enum {
        emit_doxygen,
        emit_string_constant,
    } tag;
    union {
        struct {
            char const *command;
        } doxygen;
    };
} emit_mode_t;

#define fputln(stream, str) fputs(str "\n", out)
#define fputlnf(stream, fmt, ...) fprintf(stream, fmt "\n", __VA_ARGS__)

static inline char *fgets_until_next_line(char *restrict s, int n, FILE *stream) {
    while ((s = fgets(s, n, stream)) && s[strlen(s) - 1] != '\n');
    return s;
}

static inline int embed_file(FILE *out, char const *filename, char const *const *forbidden_includes) {
    FILE *in = fopen(filename, "r");
    if (!in) {
        fputlnf(stderr, "error: cannot open %s: %s", filename, strerror(errno));
        return EX_IOERR;
    }

    char line[BUFSIZ];
    while (fgets(line, sizeof line, in)) {
        slice_t inc = extract_include(line);
        if (inc.val) {
            size_t n = arrlenu(forbidden_includes);
            bool legal = true;
            while (n-- && legal) {
                if (!strncmp(forbidden_includes[n], inc.val, inc.len)) legal = false;
            }
            if (!legal) break;
        }

        // todo: remove includes if file is in source files and not in dependencies
        fputs(line, out);
    }
    fclose(in);

    putc('\n', out);

    return EX_OK;
}

static inline int emit_file(emit_mode_t const emit, FILE *out, char const *filename) {
    FILE *in = fopen(filename, "r");
    if (!in) {
        fputlnf(stderr, "error: cannot open %s: %s", filename, strerror(errno));
        return EX_IOERR;
    }

    char line[BUFSIZ];
    if (fgets_until_next_line(line, sizeof line, in)) fputs(line, out);
    while (fgets_until_next_line(line, sizeof line, in)) {
        switch (emit.tag) {
        case emit_string_constant:
            fputs("\\n\\", out);
            break;
        case emit_doxygen:
            fputs("///", out);
            if (line[0] != '\n') putc(' ', out);
            break;
        }
        fputs(line, out);
    };

    fclose(in);
    return EX_OK;
}

static inline void emit_string(emit_mode_t const emit, FILE *out, char const *s) {
    while (*s) {
        switch (emit.tag) {
        case emit_string_constant:
            if (*s == '\n') fputs("\\n\\", out);
            putc(*s++, out);
            break;
        case emit_doxygen:
            if (*s == '\n') {
                for (; *s == '\n'; s++) fputs("\n///", out);
                putc(' ', out);
            } else {
                putc(*s++, out);
            }
            break;
        default:;
        }
    }
}

static inline int put_field(FILE *out, emit_mode_t const emit, input_field_t input) {
    if (input.tag == input_field_null) return EX_OK;
    switch (emit.tag) {
    case emit_doxygen:
        fprintf(out, "///%s ", emit.doxygen.command);
        break;
    case emit_string_constant:
        putc('"', out);
        break;
    }
    switch (input.tag) {
    case input_field_filename: {
        int e = emit_file(emit, out, input.filename);
        if (e) return e;
        break;
    }
    case input_field_string: {
        emit_string(emit, out, input.string);
        break;
    }
    default:;
    }
    switch (emit.tag) {
    case emit_doxygen:
        putc('\n', out);
        break;
    case emit_string_constant:
        putc('"', out);
        break;
    }
    return EX_OK;
}

static inline int put_field_doxygen(FILE *out, char const *command, input_field_t input) {
    return put_field(out, (emit_mode_t) { .tag = emit_doxygen, .doxygen.command = command }, input);
}

static inline int put_field_string_constant(FILE *out, input_field_t input) {
    return put_field(out, (emit_mode_t) { .tag = emit_string_constant }, input);
}

static inline char *strtoupper(char const *s) {
    char *u = strdup(s);
    char *const r = u;
    while (*s) *u++ = toupper(*s++);
    return r;
}

int library_order_deliver(library_order_t const *p_order, FILE *out) {
    fputln(out, "/// @file");
    int e = put_field_doxygen(out, " @author", p_order->author);
    if (e) return e;
    e = put_field_doxygen(out, " @brief", p_order->brief);
    if (e) return e;
    e = put_field_doxygen(out, " @copyright", p_order->copyright);
    if (e) return e;
    e = put_field_doxygen(out, " @version", p_order->version);
    if (e) return e;

    fputln(out, "/// @remark To build this, in one source file that includes this file do");
    fputln(out, "/// @code");
    char *library_uc = strtoupper(p_order->library);
#define FREE_RETURN(value) \
    do {                   \
        free(library_uc);  \
        return value;      \
    } while (0)
    fputlnf(out, "/// #define STB_%s_IMPLEMENTATION", library_uc);
    fputln(out, "/// @endcode");

    size_t n = arrlenu(p_order->dependencies);
    if (n) {
        fputln(out, "/// @remark Dependencies:");
        for (size_t i = 0; i < n; ++i) fputlnf(out, "/// - %s", p_order->dependencies[i]);
    }

    e = put_field_doxygen(out, "\n///", p_order->details);
    if (e) FREE_RETURN(e);

    fputlnf(out, "\n#ifndef STB_%s_H_INCLUDED", library_uc);
    fputlnf(out, "#define STB_%s_H_INCLUDED\n", library_uc);

    if (p_order->version.tag) {
        fprintf(out, "#define STB_%s_VERSION ", library_uc);
        e = put_field_string_constant(out, p_order->version);
        if (e) FREE_RETURN(e);
        fputln(out, "\n");
    }

    fputln(out, "#ifdef __cplusplus");
    fputln(out, "extern \"C\" {");
    fputln(out, "#endif\n");

    fputlnf(out, "#ifndef STB_%s_DEFINITION", library_uc);
    fputln(out, "/// @brief Defines modifier keywords for the definitions. Example value: static inline");
    fputlnf(out, "#define STB_%s_DEFINITION", library_uc);
    fputln(out, "#endif\n");

    // header files
    n = arrlenu(p_order->source_files);
    // possible optimization: compute and cache src_type here so we don't need to recompute it for every file for source files
    for (size_t i = 0; i < n; ++i) {
        if (src_type_detect(p_order->source_files[i]) == src_type_header) {
            embed_file(out, p_order->source_files[i], p_order->dependencies);
        }
    }

    fputln(out, "#ifdef __cplusplus");
    fputln(out, "}");
    fputln(out, "#endif\n");

    fputlnf(out, "#endif // STB_%s_H_INCLUDED\n", library_uc);

    fputlnf(out, "#ifdef STB_%s_IMPLEMENTATION\n", library_uc);

    // source files
    n = arrlenu(p_order->source_files);
    for (size_t i = 0; i < n; ++i) {
        if (src_type_detect(p_order->source_files[i]) == src_type_source) {
            embed_file(out, p_order->source_files[i], p_order->dependencies);
        }
    }

    fputlnf(out, "#endif // STB_%s_IMPLEMENTATION", library_uc);

    FREE_RETURN(e);
}

static void print_input_field(char const *name, input_field_t field) {
    printf("%s: ", name);
    switch (field.tag) {
    case input_field_null: puts("(none)"); break;
    case input_field_filename: printf("$(cat %s)\n", field.filename); break;
    case input_field_string: puts(field.string); break;
    }
}

void library_order_describe(library_order_t const *p_order) {
    printf("library: %s\n", p_order->library);

    print_input_field("author", p_order->author);
    print_input_field("brief", p_order->brief);
    print_input_field("copyright", p_order->copyright);
    print_input_field("details", p_order->details);
    print_input_field("version", p_order->version);

    printf("output: %s\n", p_order->output);

    size_t n = arrlenu(p_order->dependencies);
    printf("dependencies: %zu\n", n);
    for (size_t i = 0; i < n; ++i) {
        printf(" %zu. %s\n", i + 1, p_order->dependencies[i]);
    }

    n = arrlenu(p_order->source_files);
    printf("source files: %zu\n", n);
    for (size_t i = 0; i < n; ++i) {
        printf(" %zu. %s\n", i + 1, p_order->source_files[i]);
    }

    printf("flags:");
    if (p_order->overwrite) printf(" overwrite");
    if (p_order->check) printf(" check");
    putchar('\n');
}

#define MEMLST_IMPL

#include "lib/memlst.h"
#include "lib/stb_ds.h"
#include <assert.h>
#include <stdio.h>

struct memlst {
    void *ptr;
    dtor_fn dtor;
};

memlst_t *memlst_init() {
    return NULL;
}

void memlst_destroy(memlst_t **p_memlst FILE_LINE_PARAMS) {
#ifdef MEMLST_TRACE
    fprintf(stderr, "%s:%d memlst_destroy(%p)\n", file, line, p_memlst);
#endif
    memlst_collect(p_memlst FILE_LINE_ARGS);
    arrfree(*p_memlst);
}

void *memlst_add(memlst_t *restrict *restrict p_memlst, dtor_fn dtor, void *restrict ptr FILE_LINE_PARAMS) {
#ifdef MEMLST_TRACE
    fprintf(stderr, "%s:%d memlst_add(%p, dtor=%p, ptr=%p)\n", file, line, p_memlst, dtor, ptr);
#endif
    if (!ptr) return NULL;

#ifndef NDEBUG
    for (ptrdiff_t i = 0; i < arrlen(*p_memlst); ++i) {
        assert((*p_memlst)[i].ptr != ptr);
    }
#endif

    arrput(*p_memlst, ((memlst_t) {
                          .ptr = ptr,
                          .dtor = dtor,
                      }));
    return ptr;
}

void memlst_collect(memlst_t **p_memlst FILE_LINE_PARAMS) {
#ifdef MEMLST_TRACE
    fprintf(stderr, "%s:%d memlst_collect(%p)\n", file, line, p_memlst);
#endif
    for (ptrdiff_t i = 0; i < arrlen(*p_memlst); ++i) {
#ifdef MEMLST_TRACE
        fprintf(stderr, "  clean %p\n", (*p_memlst)[i].ptr);
#endif
        (*p_memlst)[i].dtor((*p_memlst)[i].ptr);
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    arrsetlen(*p_memlst, 0);
#pragma GCC diagnostic pop
}

/// @file
/// @author Raphaël
/// @brief Slice data structre - Implementation
/// @date 1/02/2025

#include "lib/slice.h"

#include <limits.h>

int slice_leni(slice_t slice) {
    return slice.len > INT_MAX ? INT_MAX : (int)slice.len;
}
#include "lib/library_order.h"
#include <string.h>

src_type_t src_type_detect(char const *filename) {
    char const *c = strrchr(filename, '.');
    if (!c) return src_type_others;
    switch (*++c) {
    case 'c':
        switch (*++c) {
        case '\0': return src_type_source;
        }
        return src_type_others;
    case 'h':
        switch (*++c) {
        case '\0': return src_type_header;
        }
        return src_type_others;
    }
    return src_type_others;
}

#define STB_DS_IMPLEMENTATION
#include "lib/stb_ds.h"

#include "lib/stb_stb.h"
#include "lib/memlst.h"
#include "lib/stb_ds.h"
#include "lib/library_order.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

typedef enum {
    parse_error_usage,
    // todo: add more as needed
} parse_error_t;

typedef struct {
    enum {
        parse_result_order,
        parse_result_error,
        parse_result_show_help,
        parse_result_show_version,
    } tag;
    union {
        library_order_t order;
        parse_error_t error;
    };
} parse_result_t;

static void free_order(void *ptr);

static inline parse_result_t parse_cli(int argc, char **argv);

int main(int argc, char **argv) {
    memlst_t *mem = memlst_init();
    parse_result_t pr = parse_cli(argc, argv);

    switch (pr.tag) {
    case parse_result_order:
        memlst_add(&mem, free_order, &pr.order);
        library_order_deliver(&pr.order, stdout);
        // todo: intepret order..
        CLEAN_RETURN(&mem, EX_OK);
    case parse_result_show_help:
        puts(HELP);
        CLEAN_RETURN(&mem, EX_OK);
    case parse_result_show_version:
        puts(VERSION);
        CLEAN_RETURN(&mem, EX_OK);
    case parse_result_error:
        CLEAN_RETURN(&mem, EX_USAGE);
    default: unreachable();
    }
}

parse_result_t parse_cli(int argc, char **argv) {
    enum {
        // Common options
        OPT_HELP,
        OPT_VERSION,

        // Input field options
        OPT_AUTHOR,
        OPT_FAUTHOR,
        OPT_BRIEF,
        OPT_FBRIEF,
        OPT_COPYRIGHT,
        OPT_FCOPYRIGHT,
        OPT_DETAILS,
        OPT_FDETAILS,
        OPT_FMYVERSION = 'V',
        OPT_MYVERSION = 'v',

        // Other options
        OPT_DEPENDENCY = 'd',
        OPT_OUTPUT = 'o',
        OPT_OVERWRITE,
        OPT_NO_CHECK,
    };

    struct option longopts[] = {
        {
            .name = "help",
            .val = OPT_HELP,
        },
        {
            .name = "version",
            .val = OPT_VERSION,
        },

        {
            .name = "author",
            .val = OPT_AUTHOR,
            .has_arg = required_argument,
        },
        {
            .name = "fauthor",
            .val = OPT_FAUTHOR,
            .has_arg = required_argument,
        },
        {
            .name = "brief",
            .val = OPT_BRIEF,
            .has_arg = required_argument,
        },
        {
            .name = "fbrief",
            .val = OPT_FBRIEF,
            .has_arg = required_argument,
        },
        {
            .name = "copyright",
            .val = OPT_COPYRIGHT,
            .has_arg = required_argument,
        },
        {
            .name = "fcopyright",
            .val = OPT_FCOPYRIGHT,
            .has_arg = required_argument,
        },
        {
            .name = "details",
            .val = OPT_DETAILS,
            .has_arg = required_argument,
        },
        {
            .name = "fdetails",
            .val = OPT_FDETAILS,
            .has_arg = required_argument,
        },
        {
            .name = "myversion",
            .val = OPT_VERSION,
            .has_arg = required_argument,
        },
        {
            .name = "fmyversion",
            .val = OPT_FMYVERSION,
            .has_arg = required_argument,
        },

        {
            .name = "output",
            .val = OPT_OUTPUT,
            .has_arg = required_argument,
        },
        {
            .name = "dependency",
            .val = OPT_DEPENDENCY,
            .has_arg = required_argument,
        },
        {
            .name = "overwrite",
            .val = OPT_OVERWRITE,
        },
        {
            .name = "no-check",
            .val = OPT_NO_CHECK,
        },

        { 0 },
    };

    library_order_t order = {
        .check = true,
    };

    int opt;
    while (-1 != (opt = getopt_long(argc, argv, "V:v:d:o:", longopts, NULL))) {
        switch (opt) {
        case OPT_HELP: return (parse_result_t) { .tag = parse_result_show_help };
        case OPT_VERSION: return (parse_result_t) { .tag = parse_result_show_version };

        case OPT_AUTHOR: order.author = input_field_string(optarg); break;
        case OPT_FAUTHOR: order.author = input_field_filename(optarg); break;
        case OPT_BRIEF: order.brief = input_field_string(optarg); break;
        case OPT_FBRIEF: order.brief = input_field_filename(optarg); break;
        case OPT_COPYRIGHT: order.copyright = input_field_string(optarg); break;
        case OPT_FCOPYRIGHT: order.copyright = input_field_filename(optarg); break;
        case OPT_DETAILS: order.details = input_field_string(optarg); break;
        case OPT_FDETAILS: order.details = input_field_filename(optarg); break;
        case OPT_MYVERSION: order.version = input_field_string(optarg); break;
        case OPT_FMYVERSION: order.version = input_field_filename(optarg); break;

        case OPT_OUTPUT: order.output = optarg; break;
        case OPT_DEPENDENCY:
            arrput(order.dependencies, optarg);
            break;
        case OPT_OVERWRITE: order.overwrite = true; break;
        case OPT_NO_CHECK: order.check = false; break;

        case '?':
            puts(USAGE);
            puts("see --help for details");
            return (parse_result_t) { .tag = parse_result_error, .error = parse_error_usage };
        default:
            unreachable();
        }
    }

    if (optind < argc) {
        order.library = argv[optind++];
    }
    if (optind >= argc) {
        fputs(PROG ": error: expected at least two positional arguments, LIBRARY and at least one source file\n", stderr);
        return (parse_result_t) { .tag = parse_result_error, .error = parse_error_usage };
    }
    do {
        arrput(order.source_files, argv[optind++]);
    } while (optind < argc);

    return (parse_result_t) { .tag = parse_result_order, .order = order };
}

void free_order(void *ptr)
{
    arrfree(((library_order_t*)ptr)->dependencies);
    arrfree(((library_order_t*)ptr)->source_files);
}
#endif // STB_STB_STB_IMPLEMENTATION
