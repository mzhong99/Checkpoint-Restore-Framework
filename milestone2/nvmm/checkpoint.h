#ifndef __CHECKPOINT_H__
#define __CHECKPOINT_H__

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

#include "vaddrlist.h"
#include "vtslist.h"

/**
 * A volatile checkpoint object. First, construct a new checkpoint object using
 * the [checkpoint_new()] function. Add regions of relevant data to checkpoint 
 * by using [checkpoint_add()]. Finally, commit the checkpoint by using the
 * [checkpoint_commit()] function. 
 * 
 * A commit is a blocking operation. When a checkpoint is under commit, it is
 * first sent to a background checkpoint worker task. This background worker
 * services checkpoint requests asynchronously. 
 * 
 * We specifically need an asynchronous approach with NO work stealing because 
 * we want to ensure that the calling thread's stack is preserved upon 
 * checkpoint - this in effect means that once [checkpoint_commit] is called,
 * the calling thread needs to sleep until the checkpoint is finished.
 * 
 * Once a checkpoint is finished, the calling thread unblocks, and execution can
 * continue as usual. The calling thread is free to either delete the checkpoint
 * object. Alternatively, the calling thread can also add more regions to this
 * checkpoint object before requesting another commit.
 * 
 * Two user threads should NOT attempt to modify the same checkpoint object by
 * calling [checkpoint_add()] at the same time. Checkpoints should be "owned"
 * by a single thread.
 */

struct checkpoint
{
    struct vaddrlist *addrs;
    struct vtslist_elem tselem;

    bool finished;
    bool is_kill_message;

    pthread_mutex_t lock;
    pthread_cond_t cond_finished;
};

struct checkpoint *checkpoint_new();
void checkpoint_delete(struct checkpoint *checkpoint);

void checkpoint_add(struct checkpoint *checkpoint, void *addr, size_t len);
void checkpoint_commit(struct checkpoint *checkpoint);

#endif