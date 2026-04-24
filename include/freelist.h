#ifndef FREELIST_H
#define FREELIST_H

#include "block.h"

void            freelist_init(void);
block_header_t *freelist_find(size_t size);
void            freelist_insert(block_header_t *block);
void            freelist_remove(block_header_t *block);

#endif
