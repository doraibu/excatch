#ifndef EXCATCH_H
#define EXCATCH_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// --- INFRA RAII ---
typedef void (*dtor_t)(void *);

// Sequestramos o bit 0. 
// O ponteiro na stack DEVE ser volatile para o Scanner achá-lo na RAM.
#define RAII_TAG(p)   ((void*)((uintptr_t)(p) | 0x1))
#define RAII_UNTAG(p) ((void*)((uintptr_t)(p) & ~0x7))
#define RAII_IS_TAGGED(p) ((uintptr_t)(p) & 0x1)

typedef struct { dtor_t dtor; } _raii_hdr;

// RAII_VAR agora força o 'volatile' e o 'cleanup'
#define RAII_VAR __attribute__((cleanup(_raii_internal_cleanup))) volatile void*

static inline void raii_free(void *tagged) {
    void *raw = RAII_UNTAG(tagged);
    _raii_hdr *hdr = (_raii_hdr *)raw - 1;
    if (hdr->dtor) hdr->dtor(raw);
    free(hdr);
}

// Cleanup automático para quando a variável sai de escopo normalmente
static inline void _raii_internal_cleanup(void *p) {
    void *tagged = *(void **)p;
    if (tagged && RAII_IS_TAGGED(tagged)) {
        raii_free(tagged);
        *(void **)p = NULL; // Proteção contra double-free
    }
}

static inline void* raii_malloc(size_t size, dtor_t dtor) {
    _raii_hdr *hdr = malloc(sizeof(_raii_hdr) + size);
    if (!hdr) return NULL;
    hdr->dtor = dtor;
    return RAII_TAG(hdr + 1);
}

// --- INFRA EXCEPTION ---
struct exc_slot {
    uint64_t rip, rsp, rbp, rbx, r12, r13, r14, r15;
    char func_name[32];
    int is_active;
};

#ifdef EXCATCH_IMPLEMENTATION
struct exc_slot _exc_arena[1024] = {0};
#else
extern struct exc_slot _exc_arena[1024];
#endif

int _exc_manage(const char* name, int action);

// O UNWINDER: Escaneia a pilha física em busca de tags
static inline void _unwind_stack(uint64_t current_rsp, uint64_t target_rsp) {
    uintptr_t *ptr = (uintptr_t *)current_rsp;
    while ((uint64_t)ptr < target_rsp) {
        // Se acharmos o bit 1 na stack, é um RAII_VAR
        if (RAII_IS_TAGGED(*ptr)) {
            raii_free((void *)*ptr);
            *ptr = 0; 
        }
        ptr++;
    }
}

__attribute__((noreturn)) void exc_throw(const char* name, int code) {
    int idx = _exc_manage(name, 1);
    uintptr_t target_rsp = _exc_arena[idx].rsp;
    uintptr_t current_rsp;
    
    asm volatile ("movq %%rsp, %0" : "=r"(current_rsp));
    
    _unwind_stack(current_rsp, target_rsp);

    asm volatile (
        "movq %1, %%rbx; movq %2, %%r12; movq %3, %%r13 \n\t"
        "movq %4, %%r14; movq %5, %%r15; movq %6, %%rbp \n\t"
        "movq %7, %%rsp \n\t"
        "movl %8, %%eax \n\t"
        "jmp *%0"
        : : "r"(_exc_arena[idx].rip), "m"(_exc_arena[idx].rbx), "m"(_exc_arena[idx].r12),
            "m"(_exc_arena[idx].r13), "m"(_exc_arena[idx].r14), "m"(_exc_arena[idx].r15),
            "m"(_exc_arena[idx].rbp), "m"(_exc_arena[idx].rsp), "r"(code) : "rax"
    );
    __builtin_unreachable();
}

static inline void _auto_close_try(int *idx) { 
    if (*idx != -1) _exc_manage("", 2); 
}

#define try \
    { \
        __attribute__((cleanup(_auto_close_try))) int _idx = _exc_manage(__func__, 0); \
        volatile int _err = 0; \
        asm volatile ( \
            "leaq 1f(%%rip), %%rax \n\t" \
            "movq %%rax, %0; movq %%rsp, %1; movq %%rbp, %2 \n\t" \
            "movq %%rbx, %3; movq %%r12, %4; movq %%r13, %5 \n\t" \
            "movq %%r14, %6; movq %%r15, %7 \n\t" \
            "jmp 2f \n\t" \
            "1: \n\t" \
            "movl %%eax, %8 \n\t" \
            "2: \n\t" \
            : "=m"(_exc_arena[_idx].rip), "=m"(_exc_arena[_idx].rsp), "=m"(_exc_arena[_idx].rbp), \
              "=m"(_exc_arena[_idx].rbx), "=m"(_exc_arena[_idx].r12), "=m"(_exc_arena[_idx].r13), \
              "=m"(_exc_arena[_idx].r14), "=m"(_exc_arena[_idx].r15), "=r"(_err) : : "rax", "memory" \
        ); \
        if (_err == 0)

#define catch else

#ifdef EXCATCH_IMPLEMENTATION
static uint32_t _exc_hash(const char *str) {
    uint32_t hash = 5381;
    while (*str) hash = ((hash << 5) + hash) + *str++;
    return hash % 1024;
}

int _exc_manage(const char* name, int action) {
    uint32_t idx = _exc_hash(name);
    while (_exc_arena[idx].is_active && strcmp(_exc_arena[idx].func_name, name) != 0)
        idx = (idx + 1) % 1024;
    if (action == 0) {
        _exc_arena[idx].is_active = 1;
        strncpy(_exc_arena[idx].func_name, name, 31);
    } else if (action == 2) {
        _exc_arena[idx].is_active = 0;
    }
    return idx;
}
#endif

#endif
