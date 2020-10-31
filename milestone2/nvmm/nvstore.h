#ifndef __NVSTORE_H__
#define __NVSTORE_H__

#include <stddef.h>
#include <pthread.h>

#include "list.h"
#include "crmalloc.h"
#include "checkpoint.h"

#define E_NVFS          40
#define E_UFFDOPEN      41
#define E_KSWOPEN       42
#define E_MMAP          43
#define E_IOCTL         44
#define E_PTHREAD       45
#define E_WRITE         46
#define E_CLOSE         47

/**
 * Metadata stored in non-volatile storage. You can assume that members in this
 * struct are coherent between shutdown and restarts, meaning you should NOT 
 * reinitialize any members manually upon restarts to non-empty heaps. The 
 * initialization procedure can be found in the private static function 
 * [nvstore_initmeta()].
 * 
 * One exception to the above: We assume that between checkpoints, the 
 * metadata's thread-safe elements are always unlocked. This means two major 
 * things:
 * 
 *  1.) You cannot checkpoint in the midst of spawning a thread, mutex, or any 
 *      other system resource.
 * 
 *  2.) Each system resource managing the data structures in nvmetadata itself
 *      requires a reinitialization upon a restart. This means, for example, 
 *      that the [mutexlock] and [threadlock] each require a reconstruction with
 *      [pthread_mutex_create()], but that [mutexlist] and [threadlist] do NOT
 *      require reconstruction.
 */
struct nvmetadata
{
    uint32_t writelock; /* set to 0 when writing and reset to 1 when done */
    struct memory_manager mm;       /* memory manager container               */

    pthread_mutex_t threadlock;     /* lock for thread handles container      */
    struct list threadlist;         /* list of thread handles                 */

    pthread_mutex_t mutexlock;      /* lock for mutex handles container       */
    struct list mutexlist;          /* list of mutex handles                  */
};

/** Used to manually control the write lock for the metadata */
void nvmetadata_lock(struct nvmetadata *meta);
void nvmetadata_unlock(struct nvmetadata *meta);

/** (Blocking) Checkpoints the metadata presented. */
void nvmetadata_checkpoint(struct nvmetadata *meta);

/** Singleton getter instance for global metadata */
struct nvmetadata *nvmetadata_instance();

/** Actual API for nvstore itself, includes CRUD operations */
int nvstore_init(const char *filename);
int nvstore_shutdown();

void *nvstore_allocpage(size_t npages);
void nvstore_checkpoint_everything();

/** Checkpoint only the region specified */
void nvstore_submit_checkpoint(struct checkpoint *checkpoint);

#endif
