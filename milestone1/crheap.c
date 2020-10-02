#include "crheap.h"
#include "list.h"

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/
#define CRPRINTF_BUFLEN     256

struct tagdata
{
    bool allocated: 1;
    size_t plsize: 31;
};

union tag
{
    struct tagdata data;
    volatile void *align;
};

struct block
{
    union tag tag;
    struct list_elem nvelem;
    struct list_elem velem;
    char payload[0];
};

struct crheap
{
    /* Bookkeeping system variables */
    const char *nvfilename;
    int nvfd;

    /* Volatile memory management */
    void *vheapstart;
    void *vheapend;

    /* Non-volatile mirrored members */
    void *nvheapstart;
    void *nvheapend;
};

/** Static variables for holding heap state. */
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

}

int crprintf(const char * __restrict fmt, ...)
{
    static char buffer[CRPRINTF_BUFLEN];
    va_list list;
    int i, nwrite;

    va_start(list, fmt);
    nwrite = vsnprintf(buffer, sizeof(buffer), fmt, list);
    va_end(list);

    return write(STDOUT_FILENO, buffer, nwrite);
}

int crheap_checkpoint()
{

}
