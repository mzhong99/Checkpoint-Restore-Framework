#include "crheap.h"
#include "list.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "nvstore.h"

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/
#define CRPRINTF_BUFLEN     256
#define DEFAULT_NVFILE      "heapfile.heap"

#define container_of(PTR, STRUCT_TYPE, MEMBER_NAME) \
    (STRUCT_TYPE *)((uint8_t *)(PTR) - offsetof(STRUCT_TYPE, MEMBER_NAME))

struct block
{
    bool allocated;
    size_t size;
    struct list_elem elem;
    char payload[0];
};

struct crheap
{
    struct list freelist;
};

/** Static variables for holding heap state. (use like it's an object) */
static struct crheap s_crheap;
static struct crheap *self = &s_crheap;

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/

static struct block *crheap_findfree(size_t size)
{
    struct list_elem *elem;
    struct block *block;

    elem = list_begin(&self->freelist);
    while (elem != list_end(&self->freelist))
    {
        block = list_entry(elem, struct block, elem);

        if (block->size >= size)
            return block;

        elem = list_next(elem);
    }

    return NULL;
}

/******************************************************************************/
/** Public-Facing API: Common ----------------------------------------------- */
/******************************************************************************/
int crheap_init(const char *filename)
{
    int rc;

    if (filename == NULL)
        filename = DEFAULT_NVFILE;

    rc = nvstore_init(filename);
    if (rc != 0)
        return rc;

    list_init(&self->freelist);

    return 0;
}

int crheap_shutdown()
{
    int rc;

    rc = nvstore_shutdown();
    if (rc != 0)
        return rc;

    return 0;
}

int crprintf(const char * __restrict fmt, ...)
{
    static char buffer[CRPRINTF_BUFLEN];
    va_list list;
    int nwrite;

    va_start(list, fmt);
    nwrite = vsnprintf(buffer, sizeof(buffer), fmt, list);
    va_end(list);

    return write(STDOUT_FILENO, buffer, nwrite);
}

void *crmalloc(size_t size)
{
    size_t npages;
    struct block *block;

    block = crheap_findfree(size);
    if (block != NULL)
    {
        block->allocated = true;
        list_remove(&block->elem);

        return block->payload;
    }

    npages = (size / sysconf(_SC_PAGE_SIZE)) + 1;

    block = nvstore_allocpage(npages);

    block->size = (npages * sysconf(_SC_PAGE_SIZE)) - sizeof(block);
    block->allocated = true;
    return block->payload;
}

void crfree(void *ptr)
{
    struct block *block;
    
    block = container_of(ptr, struct block, payload);
    block->allocated = false;
    list_push_back(&self->freelist, &block->elem);
}

int crheap_checkpoint()
{
    return 0;
}
