#define _GNU_SOURCE

#include <unistd.h>     /* sysconf() */
#include <assert.h>     /* assert() */
#include <fcntl.h>      /* open(), close(), posix_fallocate() */

#include <sys/types.h>  
#include <sys/stat.h>
#include <sys/mman.h>   /* mmap(), msync(), munmap(), mremap() */

#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/
#define PAGE_SIZE       ((size_t)sysconf(_SC_PAGESIZE))
#define ONE_GIGABYTE    (1U << 30)

struct crheap 
{
    void *vheapstart;
    void *vheapend;
    void *vheapcurr;
    size_t vheaplen;
    size_t npages;
};

static inline struct crheap *crheap_get_instance();
void *crheap_extend(size_t size);

/******************************************************************************/
/** Private Functions ------------------------------------------------------- */
/******************************************************************************/

static inline struct crheap *crheap_get_instance()
{
    static struct crheap crheap;
    static bool firstcall = true;

    if (firstcall)
    {
        crheap.npages = 1;
        crheap.vheaplen = crheap.npages * PAGE_SIZE;
        crheap.vheapstart = mmap(NULL, crheap.vheaplen, PROT_READ | PROT_WRITE, 
                                 MAP_PRIVATE | MAP_ANON, -1, 0);
        crheap.vheapcurr = crheap.vheapstart;
        crheap.vheapend = crheap.vheapstart + crheap.vheaplen;

        printf("%p %p %p\n", 
               crheap.vheapstart, crheap.vheapcurr, crheap.vheapend);
        printf("size=%d\n", PAGE_SIZE);

        firstcall = false;
    }

    return &crheap;
}

/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/
void *crheap_extend(size_t size)
{
    struct crheap *heap = crheap_get_instance();
    void *new_vheapcurr, *returnaddr;
    size_t remainder, npages_new;
    off_t offset;

    if (size == 0)
        return NULL;

    new_vheapcurr = heap->vheapcurr + size;
    returnaddr = heap->vheapcurr;
    offset = (off_t)(heap->vheapend - heap->vheapstart);

    if (new_vheapcurr > heap->vheapend)
    {
        printf("HERE\n");
        remainder = (size_t) (heap->vheapend - heap->vheapcurr);
        npages_new = (remainder / PAGE_SIZE) + 1;
        heap->vheapstart = mremap(heap->vheapstart, PAGE_SIZE * heap->npages, 
                                  PAGE_SIZE * (heap->npages + npages_new), 0);

        assert(heap->vheapstart != MAP_FAILED);
        heap->vheapend += npages_new * PAGE_SIZE;
        printf("%p\n", heap->vheapend);
    }

    return returnaddr;
}

void crmalloc_init()
{
    assert(crheap_get_instance() != NULL);
}

void crmalloc_test()
{
    char *c5 = crheap_extend(1 << 16);
    uint64_t iter = 0;
    while (iter < (1 << 16))
        c5[iter++] = (char)rand();
}

int main(int argc, char **argv)
{
    crmalloc_init();

    printf("Running tests...");
    crmalloc_test();
    printf("DONE\n");
}
