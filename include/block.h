#ifndef BLOCK_H
#define BLOCK_H

#include <stddef.h>
#include <stdbool.h>

#define ALIGNMENT       16
#define ALIGN(size)     (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define HEADER_SIZE     ALIGN(sizeof(block_header_t))

#define HEAP_CHUNK_SIZE (1 << 20)

typedef struct block_header {
    size_t              size;
    bool                is_free;
    struct block_header *next_free;
    struct block_header *prev_free;
} block_header_t;

#define BLOCK_PAYLOAD(header)   ((void *)((char *)(header) + HEADER_SIZE))
#define BLOCK_HEADER(payload)   ((block_header_t *)((char *)(payload) - HEADER_SIZE))
#define BLOCK_NEXT(header)      ((block_header_t *)((char *)(header) + HEADER_SIZE + (header)->size))

block_header_t *block_split(block_header_t *block, size_t size);
block_header_t *block_coalesce(block_header_t *block);
bool            block_can_split(block_header_t *block, size_t size);

#endif
