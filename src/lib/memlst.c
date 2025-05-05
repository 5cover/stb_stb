#define MEMLST_IMPL

#include "memlst.h"
#include "stb_ds.h"
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
