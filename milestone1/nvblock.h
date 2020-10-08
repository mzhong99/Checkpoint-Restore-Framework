#ifndef __NVBLOCK_H__
#define __NVBLOCK_H__

#include "list.h"
#include <stddef.h>

/* manages data concerning an mmap allocation */
struct nvblock
{
    struct list_elem elem;  /* so that we can insert into a linked list */
    size_t npages;          /* number of pages allocated in this block */

    void *pgstart;          /* the actual start of the page */
};

/* constructor and destructor functions for a non-volatile block */
struct nvblock *nvblock_new(void *pgaddr, size_t npages);
void nvblock_delete(struct nvblock *block);

#endif
