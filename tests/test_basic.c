#include "framework.h"
#include "allocator.h"

#include <stdint.h>
#include <string.h>

// ── malloc ────────────────────────────────────────────────────────────────────

TEST(malloc_returns_non_null) {
    void *p = my_malloc(64);
    ASSERT_NOT_NULL(p);
    my_free(p);
}

TEST(malloc_zero_returns_null) {
    // Standard: malloc(0) may return NULL or a unique pointer —
    // our implementation returns NULL.
    void *p = my_malloc(0);
    ASSERT_NULL(p);
}

TEST(malloc_is_aligned) {
    // Returned pointer must be 16-byte aligned
    void *p = my_malloc(1);
    ASSERT_NOT_NULL(p);
    ASSERT_EQ((uintptr_t)p % 16, 0);
    my_free(p);
}

TEST(malloc_is_writable) {
    // Write and read back every byte — catches bad sbrk size or header overlap
    size_t sz = 256;
    unsigned char *p = my_malloc(sz);
    ASSERT_NOT_NULL(p);
    memset(p, 0xAB, sz);
    ASSERT_MEM(p, 0xAB, sz);
    my_free(p);
}

TEST(malloc_multiple_independent) {
    // Two live allocations must not overlap
    unsigned char *a = my_malloc(64);
    unsigned char *b = my_malloc(64);
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);
    memset(a, 0xAA, 64);
    memset(b, 0xBB, 64);
    ASSERT_MEM(a, 0xAA, 64);   // b's writes must not have clobbered a
    ASSERT_MEM(b, 0xBB, 64);
    my_free(a);
    my_free(b);
}

TEST(malloc_large) {
    size_t sz = 1024 * 1024;   // 1 MiB
    void *p = my_malloc(sz);
    ASSERT_NOT_NULL(p);
    memset(p, 0, sz);          // should not segfault
    my_free(p);
}

TEST(malloc_many_small) {
    // Ensures the heap can service many small requests without running out
    void *ptrs[512];
    for (int i = 0; i < 512; i++) {
        ptrs[i] = my_malloc(16);
        ASSERT_NOT_NULL(ptrs[i]);
    }
    for (int i = 0; i < 512; i++)
        my_free(ptrs[i]);
}

// ── free ──────────────────────────────────────────────────────────────────────

TEST(free_null_is_noop) {
    // Must not crash
    my_free(NULL);
    ASSERT(1);
}

TEST(free_allows_reuse) {
    // After freeing, the same memory should be returned for an equal request
    void *a = my_malloc(64);
    ASSERT_NOT_NULL(a);
    my_free(a);
    void *b = my_malloc(64);
    ASSERT_NOT_NULL(b);
    // Not a hard requirement, but confirms the free list is being used
    ASSERT_EQ(a, b);
    my_free(b);
}

// ── calloc ────────────────────────────────────────────────────────────────────

TEST(calloc_zeroes_memory) {
    unsigned char *p = my_calloc(128, 1);
    ASSERT_NOT_NULL(p);
    ASSERT_MEM(p, 0x00, 128);
    my_free(p);
}

TEST(calloc_zero_count_returns_null) {
    void *p = my_calloc(0, 64);
    ASSERT_NULL(p);
}

TEST(calloc_zero_size_returns_null) {
    void *p = my_calloc(64, 0);
    ASSERT_NULL(p);
}

TEST(calloc_overflow_returns_null) {
    // nmemb * size overflows size_t
    void *p = my_calloc(SIZE_MAX, 2);
    ASSERT_NULL(p);
}

// ── realloc ───────────────────────────────────────────────────────────────────

TEST(realloc_null_acts_as_malloc) {
    void *p = my_realloc(NULL, 64);
    ASSERT_NOT_NULL(p);
    my_free(p);
}

TEST(realloc_zero_acts_as_free) {
    void *p = my_malloc(64);
    ASSERT_NOT_NULL(p);
    void *q = my_realloc(p, 0);
    ASSERT_NULL(q);
    // p should be freed — no leak (verified by stress/valgrind)
}

TEST(realloc_grow_preserves_data) {
    unsigned char *p = my_malloc(64);
    ASSERT_NOT_NULL(p);
    memset(p, 0xCD, 64);
    unsigned char *q = my_realloc(p, 128);
    ASSERT_NOT_NULL(q);
    // Original 64 bytes must be intact
    ASSERT_MEM(q, 0xCD, 64);
    my_free(q);
}

TEST(realloc_shrink_preserves_data) {
    unsigned char *p = my_malloc(256);
    ASSERT_NOT_NULL(p);
    memset(p, 0xEF, 256);
    unsigned char *q = my_realloc(p, 64);
    ASSERT_NOT_NULL(q);
    ASSERT_MEM(q, 0xEF, 64);
    my_free(q);
}

int main(void) {
    SUITE("malloc");
    RUN(malloc_returns_non_null);
    RUN(malloc_zero_returns_null);
    RUN(malloc_is_aligned);
    RUN(malloc_is_writable);
    RUN(malloc_multiple_independent);
    RUN(malloc_large);
    RUN(malloc_many_small);

    SUITE("free");
    RUN(free_null_is_noop);
    RUN(free_allows_reuse);

    SUITE("calloc");
    RUN(calloc_zeroes_memory);
    RUN(calloc_zero_count_returns_null);
    RUN(calloc_zero_size_returns_null);
    RUN(calloc_overflow_returns_null);

    SUITE("realloc");
    RUN(realloc_null_acts_as_malloc);
    RUN(realloc_zero_acts_as_free);
    RUN(realloc_grow_preserves_data);
    RUN(realloc_shrink_preserves_data);

    REPORT();
}
