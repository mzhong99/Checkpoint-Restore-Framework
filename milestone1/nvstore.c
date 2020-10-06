#include "nvstore.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <syscall.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/

struct nvstore
{
    struct list chunks;
    int killfd, uffd, nvfd;


};


/** Static variables for holding heap state. (use like it's an object) */
static struct nvstore s_nvstore;
static struct nvstore *self = &s_nvstore;

/** A getter to return the static memory management object instance */
static struct nvstore *nvstore_instance();

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/


/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/
int nvstore_init(const char *filename)
{
    self->nvfd = open(filename, O_RDWR | O_CREAT);

    if (self->nvfd == -1)
        return -EBADF;


}

int nvstore_shutdown()
{
    int rc;

    rc = close(self->nvfd);

    if (rc == -1)
        return -EBADF;

    return 0;
}
