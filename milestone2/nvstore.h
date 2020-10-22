#ifndef __NVSTORE_H__
#define __NVSTORE_H__

#include <stddef.h>
#include <pthread.h>

#include "list.h"

#define E_NVFS          40
#define E_UFFDOPEN      41
#define E_KSWOPEN       42
#define E_MMAP          43
#define E_IOCTL         44
#define E_PTHREAD       45
#define E_WRITE         46
#define E_CLOSE         47

/* metadata for non-volatile storage - each element is saved to file */
struct nvmetadata
{
    uint32_t writelock; /* set to 0 when writing and reset to 1 when done */

    pthread_mutex_t threadlock;     /* lock for thread handles container */
    struct list threadlist;         /* list of thread handles */

    pthread_mutex_t mutexlock;      /* lock for mutex handles container */
    struct list mutexlist;          /* list of mutex handles */
};

/* used to manually control the write lock for the metadata */
void nvmetadata_lock(struct nvmetadata *meta);
void nvmetadata_unlock(struct nvmetadata *meta);

/* singleton getter instance for global metadata */
struct nvmetadata *nvmetadata_instance();

/* actual API for nvstore itself, includes CRUD operations */
int nvstore_init(const char *filename);
int nvstore_shutdown();

void *nvstore_allocpage(size_t npages);
void nvstore_checkpoint();

#endif
