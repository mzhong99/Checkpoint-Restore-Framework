#include "vblock.h"
#include "memcheck.h"

#include <unistd.h>
#include <sys/mman.h>

#include <assert.h>
#include <alloca.h>
#include <string.h>

#include <stddef.h>
#include <stdio.h>

struct vblock *vblock_new(void *pgaddr, size_t npages, off_t offset)
{
    struct vblock *block = NULL;

    assert(npages > 0);
    block = mcmalloc(sizeof(*block));

    block->offset = offset;
    block->offset_pgstart = block->offset 
        + sizeof(block->pgstart) + sizeof(block->npages);

    block->npages = npages;
    block->pgstart = mcmmap(pgaddr, npages * sysconf(_SC_PAGE_SIZE), 
                             PROT_READ | PROT_WRITE | PROT_EXEC, 
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (pgaddr != NULL)
    {
        if (pgaddr != block->pgstart)
            printf("pgaddr=%p, pgstart=%p\n", pgaddr, block->pgstart);
        assert(pgaddr == block->pgstart);
    }

    return block;
}

void vblock_delete(struct vblock *block)
{
    mcmunmap(block->pgstart, block->npages * sysconf(_SC_PAGE_SIZE));
    mcfree(block);
}

off_t vblock_nvfsize(struct vblock *block)
{
    return sizeof(block->pgstart) 
         + sizeof(block->npages) 
         + (sysconf(_SC_PAGE_SIZE) * block->npages);
}

off_t vblock_pgoffset(struct vblock *block, void *addr)
{
    uintptr_t addrul, pgstartul;
    off_t frompgstart, trueoffset;

    addrul = (uintptr_t)addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
    pgstartul = (uintptr_t)block->pgstart;

    frompgstart = (off_t)(addrul - pgstartul);
    trueoffset = block->offset_pgstart + frompgstart;

    return trueoffset;
}

void vblock_dumptofile(struct vblock *block, FILE *file)
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

void vblock_dumpbypage(struct vblock *block, FILE *file, void *addr)
{
    off_t pgoffset, nwrite;
    void *pgstart, *pgcpy;

    pgstart = (void *)((uintptr_t)addr & ~(sysconf(_SC_PAGE_SIZE) - 1));
    pgcpy = alloca(sysconf(_SC_PAGE_SIZE));
    memcpy(pgcpy, pgstart, sysconf(_SC_PAGE_SIZE));

    pgoffset = vblock_pgoffset(block, pgstart);
    fseek(file, pgoffset, SEEK_SET);
    nwrite = fwrite(pgstart, 1, sysconf(_SC_PAGE_SIZE), file);
    assert(nwrite == sysconf(_SC_PAGE_SIZE));
    fflush(file);

    madvise(pgstart, sysconf(_SC_PAGE_SIZE), MADV_DONTNEED);
    memcpy(pgstart, pgcpy, sysconf(_SC_PAGE_SIZE));
}
