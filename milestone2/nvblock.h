#ifndef __NVBLOCK_H__
#define __NVBLOCK_H__

#include "list.h"

#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>

/* manages data concerning an mmap allocation */
struct nvblock
{
    struct list_elem elem;  /* so that we can insert into a linked list */
    size_t npages;          /* number of pages allocated in this block */
    void *pgstart;          /* the actual start of the page */

    off_t offset;           /* offset in file where block data is stored */
    off_t offset_pgstart;   /* offset in file where page data is stored */
};

/* constructor and destructor functions for a non-volatile block */
struct nvblock *nvblock_new(void *pgaddr, size_t npages, off_t offset);
void nvblock_delete(struct nvblock *block);

off_t nvblock_nvfsize(struct nvblock *block);
off_t nvblock_pgoffset(struct nvblock *block, void *addr);

void nvblock_dumptofile(struct nvblock *block, FILE *file);
void nvblock_dumpbypage(struct nvblock *block, FILE *file, void *addr);

#endif
