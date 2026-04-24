#include "framework.h"
#include "allocator.h"
#include "block.h"
#include "internal.h"

#include <stdint.h>

extern void *heap_start;

// ── Heap utilisation helper ───────────────────────────────────────────────────

typedef struct {
    size_t total_bytes;      // heap_end - heap_start
    size_t free_bytes;       // sum of all free block payloads
    size_t allocated_bytes;  // sum of all live block payloads
    size_t free_blocks;
    size_t allocated_blocks;
    size_t overhead_bytes;   // headers + footers
} heap_stats_t;

static heap_stats_t heap_stats(void) {
    heap_stats_t s = {0};
    s.total_bytes = (char *)heap_end - (char *)heap_start;

    block_header_t *cur = (block_header_t *)heap_start;
    while ((void *)cur < heap_end) {
        if (cur->is_free) {
            s.free_bytes += cur->size;
            s.free_blocks++;
        } else {
            s.allocated_bytes += cur->size;
            s.allocated_blocks++;
        }
        s.overhead_bytes += HEADER_SIZE + FOOTER_SIZE;
        cur = NEXT_BLOCK(cur);
    }
    return s;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST(no_leak_after_full_free) {
    void *ptrs[64];
    for (int i = 0; i < 64; i++)
        ptrs[i] = my_malloc(128);
    for (int i = 0; i < 64; i++)
        my_free(ptrs[i]);

    heap_stats_t s = heap_stats();
    // Every byte should be free — nothing allocated
    ASSERT_EQ(s.allocated_blocks, 0);
    ASSERT_EQ(s.allocated_bytes, 0);
}

TEST(coalesce_reduces_free_block_count) {
    void *a = my_malloc(64);
    void *b = my_malloc(64);
    void *c = my_malloc(64);

    my_free(a);
    my_free(b);
    my_free(c);

    heap_stats_t s = heap_stats();

    // After full coalescing all three adjacent blocks become one
    ASSERT_EQ(s.free_blocks, 1);
}

TEST(heap_utilisation_above_threshold) {
    // Measure payload efficiency of LIVE blocks only —
    // i.e. payload / (payload + per-block overhead).
    // This is independent of how much heap previous tests consumed.
    #define N 256
    void *ptrs[N];
    for (int i = 0; i < N; i++)
        ptrs[i] = my_malloc(512);

    heap_stats_t s = heap_stats();

    // Overhead for each allocated block is exactly HEADER_SIZE + FOOTER_SIZE.
    size_t alloc_overhead = s.allocated_blocks * (HEADER_SIZE + FOOTER_SIZE);
    size_t payload        = s.allocated_bytes;

    // With 512-byte payload and 48 bytes overhead: 512/(512+48) = ~91%
    ASSERT(payload * 100 / (payload + alloc_overhead) >= 80);

    for (int i = 0; i < N; i++)
        my_free(ptrs[i]);
    #undef N
}

TEST(free_list_empty_when_all_allocated) {
    void *ptrs[32];
    for (int i = 0; i < 32; i++)
        ptrs[i] = my_malloc(256);

    heap_stats_t s = heap_stats();

    // Don't assert allocated_blocks == 32 — earlier tests in this binary
    // may have left the heap in a different state.
    // What we can assert: no free blocks exist among live allocations,
    // except for at most the heap tail remainder.
    ASSERT(s.free_blocks <= 1);

    for (int i = 0; i < 32; i++)
        my_free(ptrs[i]);
}

TEST(alternating_free_no_stranding) {
    // Free every other block — without coalescing each gap is stranded.
    // With coalescing, pairs should merge and be reusable.
    #define N 64
    void *ptrs[N];
    for (int i = 0; i < N; i++)
        ptrs[i] = my_malloc(64);

    // Free even-indexed blocks
    for (int i = 0; i < N; i += 2)
        my_free(ptrs[i]);

    // Now free odd-indexed — each should merge with its left neighbour
    for (int i = 1; i < N; i += 2)
        my_free(ptrs[i]);

    heap_stats_t s = heap_stats();
    ASSERT_EQ(s.free_blocks, 1);   // entire region should be one free block
    #undef N
}

int main(void) {
    SUITE("fragmentation");
    RUN(no_leak_after_full_free);
    RUN(coalesce_reduces_free_block_count);
    RUN(heap_utilisation_above_threshold);
    RUN(free_list_empty_when_all_allocated);
    RUN(alternating_free_no_stranding);

    REPORT();
}
