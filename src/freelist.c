#include "freelist.h"

#include <stddef.h>

static block_header_t *free_list_head = NULL;

void freelist_init(void) {
    free_list_head = NULL;
}

block_header_t *freelist_find(size_t size) {
    block_header_t *curr = free_list_head;
    while (curr) {
        if (curr->size >= size)
            return curr;
        curr = curr->next_free;
    }
    return NULL;
}

void freelist_insert(block_header_t *block) {
    block->is_free    = true;
    block->next_free  = free_list_head;
    block->prev_free  = NULL;

    if (free_list_head)
        free_list_head->prev_free = block;
    
    free_list_head = block;
}

void freelist_remove(block_header_t *block) {
    if (block->prev_free)
        block->prev_free->next_free = block->next_free;
    else
        free_list_head = block->next_free;

    if (block->next_free)
        block->next_free->prev_free = block->prev_free;

    block->next_free = NULL;
    block->prev_free = NULL;
    block->is_free   = false;
}
