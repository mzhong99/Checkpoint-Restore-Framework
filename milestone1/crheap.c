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

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/
#define CRPRINTF_BUFLEN     256
#define DEFAULT_NVFILE      "heapfile.cr"

/* Imported from the malloc project - change variables as you wish! */
struct tagdata
{
    bool allocated: 1;
    size_t plsize: 31;
};

/* Imported from the malloc project - change variables as you wish! */
union tag
{
    struct tagdata data;
    volatile void *align;
};

/* Imported from the malloc project - change variables as you wish! */
struct block
{
    union tag tag;

    struct list_elem nvelem;    /* this is probably the only variable that the
                                 * non-volatile heap really needs to function */

    struct list_elem velem;
    char payload[0];
};

struct crheap
{
    /* Volatile memory management */
    void *vheapstart;
    void *vheapend;

    /* Non-volatile mirrored members */
    void *nvheapstart;
    void *nvheapend;
};

/** Static variables for holding heap state. (use like it's an object) */
static struct crheap s_crheap;
static struct crheap *self = &s_crheap;

/** A getter to return the static memory management object instance */
static struct crheap *crheap_instance();

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/
static struct crheap *crheap_instance() 
{ 
    return self; 
}

/******************************************************************************/
/** Public-Facing API: Common ----------------------------------------------- */
/******************************************************************************/
int crheap_init(const char *filename)
{
    crheap_instance();

    if (filename == NULL)
        filename = DEFAULT_NVFILE;

    return 0;
}

int crheap_shutdown()
{
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

int crheap_checkpoint()
{
    return 0;
}
