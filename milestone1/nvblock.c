#include "nvblock.h"

#include <unistd.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <assert.h>

struct nvblock *nvblock_new(void *pgaddr, size_t npages)
{
    struct nvblock *block = NULL;

    assert(npages > 0);
    block = malloc(sizeof(*block));

    block->npages = npages;
    block->pgstart = mmap(pgaddr, npages * sysconf(_SC_PAGE_SIZE), 
                          PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 
                          -1, 0);

    if (pgaddr != NULL)
        assert(pgaddr == block->pgstart);

    return block;
}

void nvblock_delete(struct nvblock *block)
{
    munmap(block->pgstart, block->npages * sysconf(_SC_PAGE_SIZE));
    free(block);
}
