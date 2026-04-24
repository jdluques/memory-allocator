#include "block.h"
#include "freelist.h"
#include "internal.h"

#include <stddef.h>

#define MIN_BLOCK_SIZE  ALIGN(sizeof(block_header_t *) * 2)

block_header_t *block_split(block_header_t *block, size_t size) {
    block_header_t *new_block = (block_header_t *)((char *)block + HEADER_SIZE + size);
    
    new_block->size         = block->size - size - HEADER_SIZE;
    new_block->is_free      = false;
    new_block->next_free    = NULL;
    new_block->prev_free    = NULL;

    block->size = size;

    return new_block;
}

block_header_t *block_coalesce(block_header_t *block) {
    block_header_t *next = BLOCK_NEXT(block);
    
    if ((void *)next < heap_end && next->is_free) {
        freelist_remove(next);
        block->size += HEADER_SIZE + next->size;
    }
    
    return block;
}

bool block_can_split(block_header_t *block, size_t size) {
    if (block->size < size) return false;
    size_t remainder = block->size - size;
    return remainder >= HEADER_SIZE + MIN_BLOCK_SIZE;
}
