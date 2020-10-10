#include "nvblock.h"

#include <unistd.h>
#include <sys/mman.h>

#include <assert.h>

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

struct nvblock *nvblock_new(void *pgaddr, size_t npages, off_t offset)
{
    struct nvblock *block = NULL;

    assert(npages > 0);
    block = malloc(sizeof(*block));

    block->offset = offset;
    block->offset_pgstart = block->offset 
        + sizeof(block->pgstart) + sizeof(block->npages);

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

off_t nvblock_nvfsize(struct nvblock *block)
{
    return sizeof(block->pgstart) 
         + sizeof(block->npages) 
         + (sysconf(_SC_PAGE_SIZE) * block->npages);
}

off_t nvblock_pgoffset(struct nvblock *block, void *addr)
{
    uintptr_t addrul, pgstartul;
    off_t frompgstart, trueoffset;

    addrul = (uintptr_t)addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
    pgstartul = (uintptr_t)block->pgstart;

    frompgstart = (off_t)(addrul - pgstartul);
    trueoffset = block->offset_pgstart + frompgstart;

    return trueoffset;
}

void nvblock_dumptofile(struct nvblock *block, FILE *file)
{
    size_t nwrite;

    /* first, write the address */
    fseek(file, block->offset, SEEK_SET);
    nwrite = fwrite(&block->pgstart, 1, sizeof(block->pgstart), file);
    assert(nwrite == sizeof(block->pgstart));

    /* second, write the number of pages */
    fseek(file, block->offset + sizeof(block->pgstart), SEEK_SET);
    nwrite = fwrite(&block->npages, 1, sizeof(block->npages), file);
    assert(nwrite == sizeof(block->npages));

    /* third, write the actual page data */
    fseek(file, block->offset_pgstart, SEEK_SET);
    nwrite = fwrite(block->pgstart, 1, block->npages * sysconf(_SC_PAGE_SIZE), 
                    file);
    assert(nwrite == block->npages * sysconf(_SC_PAGE_SIZE));
}

void nvblock_dumpbypage(struct nvblock *block, FILE *file, void *addr)
{
    off_t pgoffset, nwrite;
    void *pgstart;

    pgstart = (void *)((uintptr_t)addr & ~(sysconf(_SC_PAGE_SIZE) - 1));

    pgoffset = nvblock_pgoffset(block, pgstart);
    fseek(file, pgoffset, SEEK_SET);
    nwrite = fwrite(pgstart, 1, sysconf(_SC_PAGE_SIZE), file);
    assert(nwrite == sysconf(_SC_PAGE_SIZE));

    madvise(pgstart, sysconf(_SC_PAGE_SIZE), MADV_DONTNEED);
}
