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
#include "vtsthreadtable.h"

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
    if (filename == NULL)
        filename = DEFAULT_NVFILE;

    nvstore_init(filename);
    crthread_init_system();

    return 0;
}

int crheap_shutdown()
{
    crheap_checkpoint_everything();
    crthread_shutdown_system();
    nvstore_shutdown();

    return 0;
}

int crheap_shutdown_nosave()
{
    nvmetadata_checkpoint(nvmetadata_instance());
    crthread_shutdown_system();
    nvstore_shutdown();

    return 0;
}

int crheap_checkpoint_everything()
{
    nvstore_checkpoint_everything();
    return 0;
}
