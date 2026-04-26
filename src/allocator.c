#define _DEFAULT_SOURCE

#include "allocator.h"
#include "block.h"
#include "freelist.h"
#include "internal.h"

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static bool heap_initialized = false;
void *heap_start    = NULL;
void *heap_end      =  NULL;

pthread_mutex_t heap_mutex = PTHREAD_MUTEX_INITIALIZER;

static void heap_init(void) {
    heap_start  = sbrk(0);
    heap_end    = heap_start;
    freelist_init();
    heap_initialized = true;
}

static block_header_t *heap_extend(size_t size) {
    size_t extend_size = size + HEADER_SIZE + FOOTER_SIZE;
    if (extend_size < HEAP_CHUNK_SIZE)
        extend_size = HEAP_CHUNK_SIZE;

    void *raw = sbrk((intptr_t)extend_size);
    if (raw == (void *)-1)
        return NULL;

    block_header_t *block = (block_header_t *)raw;
    block_set_size(block, extend_size - HEADER_SIZE - FOOTER_SIZE);
    block->is_free      = false;
    block->next_free    = NULL;
    block->prev_free    = NULL;

    heap_end = (char *)raw + extend_size;

    return block;
}

static void *mmap_alloc(size_t size) {
    size_t total = ALIGN(HEADER_SIZE + size);

    void *raw = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (raw == MAP_FAILED) return NULL;

    block_header_t *block = (block_header_t *)raw;
    block_set_size(block, size | MMAP_FLAG);
    block->is_free      = false;
    block->next_free    = NULL;
    block->prev_free    = NULL;

    return BLOCK_PAYLOAD(block);
}

static void mmap_free(block_header_t *block) {
    size_t total = ALIGN(HEADER_SIZE + BLOCK_RAW_SIZE(block));
    munmap(block, total);
}

void *my_malloc(size_t size) {
    if (size == 0) return NULL;

    pthread_mutex_lock(&heap_mutex);

    if (!heap_initialized) heap_init();

    size = ALIGN(size);

    block_header_t *block = freelist_find(size);
    if (block) {
        freelist_remove(block);
    } else {
        if (size >= MMAP_THRESHOLD) {
            pthread_mutex_unlock(&heap_mutex);
            return mmap_alloc(size);
        }
        
        block = heap_extend(size);
        if (!block) {
            pthread_mutex_unlock(&heap_mutex);
            return NULL;
        }
    }

    if (block_can_split(block, size)) {
        block_header_t *remainder = block_split(block, size);
        freelist_insert(remainder);
    }

    block->is_free = false;

    pthread_mutex_unlock(&heap_mutex);

    return BLOCK_PAYLOAD(block);
}

void my_free(void *ptr) {
    if (!ptr) return;

    block_header_t *block = BLOCK_HEADER(ptr);

    if (BLOCK_IS_MMAP(block)) {
        mmap_free(block);
        return;
    }

    pthread_mutex_lock(&heap_mutex);

    block->is_free = true;
    block = block_coalesce(block);
    
    freelist_insert(block);

    pthread_mutex_unlock(&heap_mutex);
}

void *my_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) return NULL;

    size_t total = nmemb * size;
    if (total / nmemb != size) return NULL;

    void *ptr = my_malloc(total);
    if (ptr) memset(ptr, 0, total);

    return ptr;
}

void *my_realloc(void *ptr, size_t size) {
    if (!ptr)      return my_malloc(size);
    if (size == 0) { my_free(ptr); return NULL; }

    block_header_t *block    = BLOCK_HEADER(ptr);
    size_t          raw_size = BLOCK_RAW_SIZE(block);

    if (raw_size >= size) {
        if (!BLOCK_IS_MMAP(block)){
            pthread_mutex_lock(&heap_mutex);

            if (block_can_split(block, size)) {
                block_header_t *remainder = block_split(block, size);
                freelist_insert(remainder);
            }

            pthread_mutex_unlock(&heap_mutex);
        }

        return ptr;
    }

    void *new_ptr = my_malloc(size);
    if (!new_ptr) return NULL;
    memcpy(new_ptr, ptr, block->size);
    my_free(ptr);

    return new_ptr;

}
