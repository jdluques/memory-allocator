#include "framework.h"
#include "allocator.h"
#include "block.h"

// Helpers ─────────────────────────────────────────────────────────────────────

// Returns the header of the block owning ptr
static block_header_t *hdr(void *ptr) {
    return BLOCK_HEADER(ptr);
}

// ── Right coalescing ──────────────────────────────────────────────────────────

TEST(coalesce_right_merges_adjacent) {
    // Allocate two adjacent blocks, free right then left.
    // Left-free should absorb right.
    void *a = my_malloc(64);
    void *b = my_malloc(64);
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);

    size_t a_size_before = hdr(a)->size;

    my_free(b);
    my_free(a);   // should right-coalesce with (now free) b

    // Reallocate to the combined size — must succeed from free list
    void *c = my_malloc(a_size_before * 2);
    ASSERT_NOT_NULL(c);
    // If coalescing worked, c should be at the same address as a was
    ASSERT_EQ(c, a);
    my_free(c);
}

TEST(coalesce_right_does_not_merge_allocated) {
    void *a = my_malloc(64);
    void *b = my_malloc(64);  // b stays live
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);

    size_t a_size_before = hdr(a)->size;
    my_free(a);

    // a's block must NOT grow — b is still allocated
    ASSERT_EQ(hdr(a)->size, a_size_before);

    my_free(b);
}

// ── Left coalescing ───────────────────────────────────────────────────────────

TEST(coalesce_left_merges_adjacent) {
    void *a = my_malloc(64);
    void *b = my_malloc(64);
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);

    my_free(a);   // a is free first
    my_free(b);   // b should left-coalesce with a

    // The merged block should be back at a's address
    void *c = my_malloc(hdr(a)->size);
    ASSERT_NOT_NULL(c);
    ASSERT_EQ(c, a);
    my_free(c);
}

// ── Three-way coalescing ──────────────────────────────────────────────────────

TEST(coalesce_three_way) {
    // Free in middle-first order so all three merges are exercised
    void *a = my_malloc(64);
    void *b = my_malloc(64);
    void *c = my_malloc(64);
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(c);

    my_free(b);   // b is isolated free
    my_free(c);   // c right-coalesces into b
    my_free(a);   // a left-coalesces with (b+c)

    // Should be able to alloc triple-sized block from the same address
    void *d = my_malloc(hdr(a)->size);
    ASSERT_NOT_NULL(d);
    ASSERT_EQ(d, a);
    my_free(d);
}

// ── Footer integrity ──────────────────────────────────────────────────────────

TEST(footer_mirrors_header_after_alloc) {
    void *p = my_malloc(128);
    ASSERT_NOT_NULL(p);
    block_header_t *h = hdr(p);
    block_footer_t *f = BLOCK_FOOTER(h);
    ASSERT_EQ(h->size, f->size);
    my_free(p);
}

TEST(footer_mirrors_header_after_split) {
    // Allocate large then small — the large one gets split
    void *big = my_malloc(512);
    my_free(big);
    void *small = my_malloc(32);
    ASSERT_NOT_NULL(small);
    block_header_t *h = hdr(small);
    block_footer_t *f = BLOCK_FOOTER(h);
    ASSERT_EQ(h->size, f->size);
    my_free(small);
}

TEST(footer_mirrors_header_after_coalesce) {
    void *a = my_malloc(64);
    void *b = my_malloc(64);
    my_free(b);
    my_free(a);   // triggers coalesce — merged block's footer must update
    block_header_t *h = hdr(a);
    block_footer_t *f = BLOCK_FOOTER(h);
    ASSERT_EQ(h->size, f->size);
    // Clean up
    void *c = my_malloc(h->size);
    my_free(c);
}

int main(void) {
    SUITE("right coalescing");
    RUN(coalesce_right_merges_adjacent);
    RUN(coalesce_right_does_not_merge_allocated);

    SUITE("left coalescing");
    RUN(coalesce_left_merges_adjacent);

    SUITE("three-way coalescing");
    RUN(coalesce_three_way);

    SUITE("footer integrity");
    RUN(footer_mirrors_header_after_alloc);
    RUN(footer_mirrors_header_after_split);
    RUN(footer_mirrors_header_after_coalesce);

    REPORT();
}
