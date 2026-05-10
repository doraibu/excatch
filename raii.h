#ifndef RAII_H
#define RAII_H

#include <stdlib.h>
#include <stdint.h>

typedef void (*dtor_t)(void *);

#define RAII_TAG(p)       ((void*)((uintptr_t)(p) | 0x1))
#define RAII_UNTAG(p)     ((void*)((uintptr_t)(p) & ~0x7))
#define RAII_IS_TAGGED(p) ((uintptr_t)(p) & 0x1)

typedef struct { dtor_t dtor; } _raii_hdr;

static inline void raii_free(void *tagged) {
    void *raw = RAII_UNTAG(tagged);
    _raii_hdr *hdr = (_raii_hdr *)raw - 1;
    if (hdr->dtor) hdr->dtor(raw);
    free(hdr);
}

static inline void _raii_internal_cleanup(void *p) {
    void *tagged = *(void **)p;
    if (tagged && RAII_IS_TAGGED(tagged)) {
        raii_free(tagged);
        *(void **)p = NULL;
    }
}

#define RAII_VAR __attribute__((cleanup(_raii_internal_cleanup))) volatile void*

static inline void* raii_malloc(size_t size, dtor_t dtor) {
    _raii_hdr *hdr = malloc(sizeof(_raii_hdr) + size);
    if (!hdr) return NULL;
    hdr->dtor = dtor;
    return RAII_TAG(hdr + 1);
}

#endif
