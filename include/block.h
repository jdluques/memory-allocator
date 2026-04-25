#ifndef BLOCK_H
#define BLOCK_H

#include <stddef.h>
#include <stdbool.h>

#define ALIGNMENT       16
#define ALIGN(size)     (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

typedef struct block_header {
    size_t              size;
    bool                is_free;
    struct block_header *next_free;
    struct block_header *prev_free;
} block_header_t;

typedef struct block_footer {
    size_t size;
} block_footer_t;


#define HEADER_SIZE             ALIGN(sizeof(block_header_t))
#define FOOTER_SIZE             ALIGN(sizeof(block_footer_t))
#define BLOCK_TOTAL(header)     (HEADER_SIZE + (header)->size + FOOTER_SIZE)
#define HEAP_CHUNK_SIZE         (1 << 20)

#define BLOCK_PAYLOAD(header)   ((void *)((char *)(header) + HEADER_SIZE))
#define BLOCK_HEADER(payload)   ((block_header_t *)((char *)(payload) - HEADER_SIZE))
#define BLOCK_FOOTER(header)    ((block_footer_t *)((char *)(header) + HEADER_SIZE + (header)->size))
#define PREV_FOOTER(header)     ((block_footer_t *)((char *)(header) - FOOTER_SIZE))
#define PREV_BLOCK(header)      ((block_header_t *)((char *)(header) - FOOTER_SIZE - PREV_FOOTER(header)->size - HEADER_SIZE))
#define NEXT_BLOCK(header)      ((block_header_t *)((char *)(header) + HEADER_SIZE + (header)->size + FOOTER_SIZE))

#define SIZE_MASK               (~(size_t)(ALIGNMENT - 1))
#define MMAP_FLAG               (0x1)

#define BLOCK_IS_MMAP(header)   ((header)->size & MMAP_FLAG)
#define BLOCK_RAW_SIZE(header)  ((header)->size & SIZE_MASK)

void            block_set_size(block_header_t *block, size_t size);
block_header_t *block_split(block_header_t *block, size_t size);
block_header_t *block_coalesce(block_header_t *block);
bool            block_can_split(block_header_t *block, size_t size);

#endif
