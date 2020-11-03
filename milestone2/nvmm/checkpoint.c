#include "checkpoint.h"
#include "memcheck.h"
#include "nvstore.h"
#include "crmalloc.h"

#include <stdlib.h>
#include <assert.h>

/** Creates a new checkpoint object. */
struct checkpoint *checkpoint_new()
{
    struct checkpoint *checkpoint;

    checkpoint = mc_malloc(sizeof(*checkpoint));
    checkpoint->addrs = vaddrlist_new(NVADDRLIST_INIT_POWER);

    checkpoint->is_kill_message = false;
    sem_init(&checkpoint->finished, 0, 0);

    return checkpoint;
}

/** Deletes the checkpoint object supplied. */
void checkpoint_delete(struct checkpoint *checkpoint)
{
    vaddrlist_delete(checkpoint->addrs);
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
    nvstore_submit_checkpoint(checkpoint);
    sem_wait(&checkpoint->finished);
}

/** Marks the commit as finished - should only be called by system. */
void checkpoint_post_commit_finished(struct checkpoint *checkpoint)
{
    sem_post(&checkpoint->finished);
}