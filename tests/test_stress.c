#include "framework.h"
#include "allocator.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define SLOTS       1024
#define ITERATIONS  50000
#define MAX_SIZE    4096

// Deterministic LCG so results are reproducible without <stdlib.h> rand
static uint32_t _seed = 0xDEADBEEF;
static uint32_t lcg(void) {
    _seed = _seed * 1664525u + 1013904223u;
    return _seed;
}

// ── Core stress ───────────────────────────────────────────────────────────────

TEST(stress_random_alloc_free) {
    // Each slot holds either a live pointer+size pair or NULL.
    // On each iteration we either alloc into an empty slot or free a live one.
    void  *ptrs[SLOTS] = {0};
    size_t sizes[SLOTS] = {0};

    for (int i = 0; i < ITERATIONS; i++) {
        int slot = lcg() % SLOTS;

        if (ptrs[slot]) {
            // Verify fill pattern before freeing
            ASSERT_MEM(ptrs[slot], (uint8_t)(slot & 0xFF), sizes[slot]);
            my_free(ptrs[slot]);
            ptrs[slot]  = NULL;
            sizes[slot] = 0;
        } else {
            size_t sz = (lcg() % MAX_SIZE) + 1;
            ptrs[slot] = my_malloc(sz);
            ASSERT_NOT_NULL(ptrs[slot]);
            // Alignment check
            ASSERT_EQ((uintptr_t)ptrs[slot] % 16, 0);
            // Tag with a fill pattern so we can detect overwrites
            memset(ptrs[slot], (uint8_t)(slot & 0xFF), sz);
            sizes[slot] = sz;
        }
    }

    // Clean up remaining live allocations
    for (int i = 0; i < SLOTS; i++) {
        if (ptrs[i]) {
            ASSERT_MEM(ptrs[i], (uint8_t)(i & 0xFF), sizes[i]);
            my_free(ptrs[i]);
        }
    }
}

TEST(stress_interleaved_sizes) {
    // Alternate tiny and large to maximise splitting/coalescing pressure
    void *tiny[128];
    void *large[128];

    for (int i = 0; i < 128; i++) {
        tiny[i]  = my_malloc(16);
        large[i] = my_malloc(2048);
        ASSERT_NOT_NULL(tiny[i]);
        ASSERT_NOT_NULL(large[i]);
        memset(tiny[i],  0xAA, 16);
        memset(large[i], 0xBB, 2048);
    }

    for (int i = 0; i < 128; i++) {
        ASSERT_MEM(tiny[i],  0xAA, 16);
        ASSERT_MEM(large[i], 0xBB, 2048);
        my_free(tiny[i]);
        my_free(large[i]);
    }
}

TEST(stress_realloc_chain) {
    // Grow a single allocation through many sizes
    void *p = my_malloc(16);
    ASSERT_NOT_NULL(p);
    memset(p, 0x42, 16);

    for (size_t sz = 32; sz <= 8192; sz *= 2) {
        p = my_realloc(p, sz);
        ASSERT_NOT_NULL(p);
        ASSERT_EQ((uintptr_t)p % 16, 0);
        // Fill pattern from original 16 bytes must survive every realloc
        // (only first 16 bytes are guaranteed to be preserved)
        ASSERT_MEM(p, 0x42, 16);
        memset(p, 0x42, sz);   // re-fill so next iteration can check
    }
    my_free(p);
}

TEST(stress_free_right_to_left) {
    // Classic fragmentation case — without left-coalescing this strands blocks
    #define N 128
    void *ptrs[N];
    size_t sz = 64;

    for (int i = 0; i < N; i++) {
        ptrs[i] = my_malloc(sz);
        ASSERT_NOT_NULL(ptrs[i]);
    }

    // Free right to left
    for (int i = N - 1; i >= 0; i--)
        my_free(ptrs[i]);

    // The entire region should now be one merged block;
    // a single large allocation must succeed
    void *big = my_malloc(sz * N / 2);
    ASSERT_NOT_NULL(big);
    my_free(big);
    #undef N
}

int main(void) {
    SUITE("stress");
    RUN(stress_random_alloc_free);
    RUN(stress_interleaved_sizes);
    RUN(stress_realloc_chain);
    RUN(stress_free_right_to_left);

    REPORT();
}
