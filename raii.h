#ifndef RAII_H
#define RAII_H

#include <stdlib.h>
#include <stdint.h>

typedef void (*dtor_t)(void *);

// Sequestra o bit 0 para marcar como RAII
#define RAII_TAG(p)   ((void*)((uintptr_t)(p) | 0x1))
#define RAII_UNTAG(p) ((void*)((uintptr_t)(p) & ~0x7))
#define RAII_IS_TAGGED(p) ((uintptr_t)(p) & 0x1)

// Estrutura invisível que precede o ponteiro
typedef struct {
    dtor_t dtor;
    void *data;
} _raii_hdr;

static inline void _raii_internal_cleanup(void *p) {
    void **ptr_to_tagged = (void **)p;
    if (!ptr_to_tagged || !*ptr_to_tagged || !RAII_IS_TAGGED(*ptr_to_tagged)) return;

    void *raw = RAII_UNTAG(*ptr_to_tagged);
    _raii_hdr *hdr = (_raii_hdr *)raw - 1;
    if (hdr->dtor) hdr->dtor(raw);
    free(hdr);
}

// O "Objeto" RAII na Stack: Uma variável que se auto-destrói
#define RAII_VAR __attribute__((cleanup(_raii_internal_cleanup))) void*

static inline void* raii_malloc(size_t size, dtor_t dtor) {
    _raii_hdr *hdr = malloc(sizeof(_raii_hdr) + size);
    if (!hdr) return NULL;
    hdr->dtor = dtor;
    hdr->data = (hdr + 1);
    return RAII_TAG(hdr->data);
}

#define RAII_GET(type, p) ((type*)RAII_UNTAG(p))

#endif
