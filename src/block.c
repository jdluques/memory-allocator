#include "block.h"
#include "freelist.h"
#include "internal.h"

#include <stdbool.h>
#include <stddef.h>

#define MIN_BLOCK_SIZE  ALIGN(sizeof(block_header_t *) * 2)

void block_set_size(block_header_t *block, size_t size) {
    block->size = size;
    BLOCK_FOOTER(block)->size = size;
}

block_header_t *block_split(block_header_t *block, size_t size) {
    size_t remainder_size = block->size - size - HEADER_SIZE - FOOTER_SIZE;

    block_set_size(block, size);

    block_header_t *new_block = (block_header_t *)((char *)block + HEADER_SIZE + size + FOOTER_SIZE);
    block_set_size(new_block, remainder_size);
    new_block->is_free      = false;
    new_block->next_free    = NULL;
    new_block->prev_free    = NULL;

    return new_block;
}

block_header_t *block_coalesce(block_header_t *block) {
    bool at_heap_start = ((void *)block == heap_start);

    block_header_t *next = NEXT_BLOCK(block);
    if ((void *)next < heap_end && next->is_free) {
        freelist_remove(next);
        block_set_size(block, block->size + HEADER_SIZE + FOOTER_SIZE + next->size);
    }

    if (!at_heap_start) {
        block_footer_t *prev_footer = PREV_FOOTER(block);
        block_header_t *prev = PREV_BLOCK(block);

        if (prev->is_free) {
            freelist_remove(prev);
            block_set_size(prev, prev->size + HEADER_SIZE + FOOTER_SIZE + block->size);
            block = prev;
        }
    }
    
    return block;
}

bool block_can_split(block_header_t *block, size_t size) {
    if (block->size < size) return false;
    size_t remainder_size = block->size - size;
    return remainder_size >= HEADER_SIZE + FOOTER_SIZE + MIN_BLOCK_SIZE;
}
