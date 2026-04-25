#ifndef FREELIST_H
#define FREELIST_H

#include "block.h"

// ── Size classes ──────────────────────────────────────────────────────────────
//
//  Class  Max payload
//  -----  -----------
//    0        16 B
//    1        32 B
//    2        64 B
//    3       128 B
//    4       256 B
//    5       512 B
//    6      1024 B
//    7      2048 B
//    8      4096 B
//    9      8192 B
//   10     16384 B
//   11      huge  (> 16384 B)

#define NUM_CLASSES 12

int             freelist_class(size_t size);

void            freelist_init(void);
block_header_t *freelist_find(size_t size);
void            freelist_insert(block_header_t *block);
void            freelist_remove(block_header_t *block);

#endif
