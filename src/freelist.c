#include "freelist.h"
#include "block.h"

#include <stddef.h>

int freelist_class(size_t size) {
    if (size <=    16) return  0;
    if (size <=    32) return  1;
    if (size <=    64) return  2;
    if (size <=   128) return  3;
    if (size <=   256) return  4;
    if (size <=   512) return  5;
    if (size <=  1024) return  6;
    if (size <=  2048) return  7;
    if (size <=  4096) return  8;
    if (size <=  8192) return  9;
    if (size <= 16384) return 10;
    return 11;
}

static block_header_t *bins[NUM_CLASSES];

static void bin_push(int cls, block_header_t *block) {
    block->next_free = bins[cls];
    block->prev_free = NULL;
    if (bins[cls])
        bins[cls]->prev_free = block;
    bins[cls] = block;
}

static void bin_unlink(int cls, block_header_t *block) {
    if (block->prev_free)
        block->prev_free->next_free = block->next_free;
    else
        bins[cls] = block->next_free;

    if (block->next_free)
        block->next_free->prev_free = block->prev_free;

    block->next_free = NULL;
    block->prev_free = NULL;
}

static void huge_insert(block_header_t *block) {
    block_header_t *curr = bins[NUM_CLASSES-1];
    block_header_t *prev = NULL;

    while (curr && curr < block) {
        prev = curr;
        curr = curr->next_free;
    }

    block->next_free = curr;
    block->prev_free = prev;

    if (prev) prev->next_free = block;
    else      bins[NUM_CLASSES-1] = block;

    if (curr) curr->prev_free = block;
}

static block_header_t *huge_find(size_t size) {
    block_header_t *curr = bins[NUM_CLASSES-1];

    while (curr) {
        if (curr->size >= size) return curr;
        curr = curr->next_free;
    }

    return NULL;
}

void freelist_init(void) {
    for (int i = 0; i < NUM_CLASSES; i++)
        bins[i] = NULL;
}

block_header_t *freelist_find(size_t size) {
    if (freelist_class(size) == NUM_CLASSES-1)
        return huge_find(size);

    for (int cls = freelist_class(size); cls < NUM_CLASSES-1; cls++) {
        block_header_t *curr = bins[cls];
        while (curr) {
            if (curr->size >= size) return curr;
            curr = curr->next_free;
        }
    }

    return huge_find(size);
}

void freelist_insert(block_header_t *block) {
    block->is_free    = true;
    
    int cls = freelist_class(block->size);
    
    if (cls < NUM_CLASSES-1)
        bin_push(cls, block);
    else
        huge_insert(block);
}

void freelist_remove(block_header_t *block) {
    int cls = freelist_class(block->size);
    bin_unlink(cls, block);
    block->is_free = false;
}
