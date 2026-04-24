#define _DEFAULT_SOURCE

#include "allocator.h"
#include "block.h"
#include "freelist.h"

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>

static bool   heap_initialized  = false;
static void  *heap_start        = NULL;
static void  *heap_end          = NULL;

static void heap_init(void) {
    heap_start  = sbrk(0);
    heap_end    = heap_start;
    freelist_init();
    heap_initialized = true;
}

static block_header_t *heap_extend(size_t size) {
    size_t extend_size = size + HEADER_SIZE;
    if (extend_size < HEAP_CHUNK_SIZE)
        extend_size = HEAP_CHUNK_SIZE;

    void *raw = sbrk((intptr_t)extend_size);
    if (raw == (void *)-1)
        return NULL;

    block_header_t *block = (block_header_t *)raw;
    block->size         = extend_size - HEADER_SIZE;
    block->is_free      = true;
    block->next_free    = NULL;
    block->prev_free    = NULL;
    heap_end = (char *)raw + extend_size;

    return block;
}

void *my_malloc(size_t size) {
    if (size == 0) return NULL;
    if (!heap_initialized) heap_init();

    size = ALIGN(size);

    block_header_t *block = freelist_find(size);

    if (block) {
        freelist_remove(block);
    } else {
        block = heap_extend(size);
        if (!block) return NULL;
    }

    if (block_can_split(block, size)) {
        block_header_t *remainder = block_split(block, size);
        freelist_insert(remainder);
    }

    block->is_free = false;

    return BLOCK_PAYLOAD(block);
}

void my_free(void *ptr) {
    if (!ptr) return;

    block_header_t *block = BLOCK_HEADER(ptr);
    block->is_free = true;

    block = block_coalesce(block);
    freelist_insert(block);
}

void *my_calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    if (nmemb != 0 && total / nmemb != size) return NULL;

    void *ptr = my_malloc(total);
    if (ptr) memset(ptr, 0, total);

    return ptr;
}

void *my_realloc(void *ptr, size_t size) {
    if (!ptr) return my_malloc(size);
    if (size == 0) { my_free(ptr); return NULL; }

    block_header_t *block = BLOCK_HEADER(ptr);

    if (block->size >= size) {
        if (block_can_split(block, size)) {
            block_header_t *remainder = block_split(block, size);
            freelist_insert(remainder);
        }
        return ptr;
    }

    void *new_ptr = my_malloc(size);
    if (!new_ptr) return NULL;
    memcpy(new_ptr, ptr, block->size);
    my_free(ptr);

    return new_ptr;

}
