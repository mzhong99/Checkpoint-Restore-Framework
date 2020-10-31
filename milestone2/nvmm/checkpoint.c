#include "checkpoint.h"
#include "memcheck.h"
#include "nvstore.h"

#include <stdlib.h>

/** Creates a new checkpoint object. */
struct checkpoint *checkpoint_new()
{
    struct checkpoint *checkpoint;

    checkpoint = mc_malloc(sizeof(*checkpoint));
    checkpoint->addrs = vaddrlist_new(NVADDRLIST_INIT_POWER);

    checkpoint->finished = false;
    checkpoint->is_kill_message = false;

    pthread_mutex_init(&checkpoint->lock, NULL);
    pthread_cond_init(&checkpoint->cond_finished, NULL);

    return checkpoint;
}

/** Deletes the checkpoint object supplied. */
void checkpoint_delete(struct checkpoint *checkpoint)
{
    vaddrlist_delete(checkpoint->addrs);

    pthread_cond_destroy(&checkpoint->cond_finished);
    pthread_mutex_destroy(&checkpoint->lock);

    mc_free(checkpoint);
}

/** Adds a region to be checkpointed. Specifically adds pages for the region. */
void checkpoint_add(struct checkpoint *checkpoint, void *addr, size_t len)
{
    vaddrlist_insert_pages_of(checkpoint->addrs, addr, len);
}

/** 
 * Commits the region by sending this object to the checkpoint worker and 
 * waiting until the commit job is completed.
 */
void checkpoint_commit(struct checkpoint *checkpoint)
{
    pthread_mutex_lock(&checkpoint->lock);

    nvstore_submit_checkpoint(checkpoint);

    while (!checkpoint->finished)
        pthread_cond_wait(&checkpoint->cond_finished, &checkpoint->lock);

    pthread_mutex_unlock(&checkpoint->lock);
}