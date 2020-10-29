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
#include "vthreadtable.h"

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/
#define CRPRINTF_BUFLEN     256
#define DEFAULT_NVFILE      "heapfile.heap"

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

    vthreadtable_init();

    return 0;
}

int crheap_shutdown()
{
    int rc;

    rc = crheap_checkpoint();
    if (rc != 0)
        return rc;

    rc = nvstore_shutdown();
    if (rc != 0)
        return rc;

    vthreadtable_cleanup();

    return 0;
}

int crheap_checkpoint()
{
    nvstore_checkpoint();

    return 0;
}
