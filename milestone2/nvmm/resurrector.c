#include "resurrector.h"
#include "vtslist.h"
#include "vtsthreadtable.h"
#include "crheap.h"
#include "memcheck.h"

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/

/** Commands which the resurrector can actively process. */
enum resurrector_command { RESURRECTOR_CHECKPOINT, RESURRECTOR_SHUTDOWN };

/** Message format of input messages into resurrector thread module. */
struct resurrector_msg
{
    enum resurrector_command cmd;   /* Which command to perform. */
    struct vtslist_elem vtselem;    /* List element. */
    struct crthread *thread;        /* Thread under which to operate. */
};

/** Resurrector object context. */
struct resurrector
{
    struct vtslist input;   /* Input queue of messages. */
    pthread_t resworker;    /* Worker thread handle. */
};

/* Static singleton instance. */
static struct resurrector s_resurrector;
static struct resurrector *self = &s_resurrector;

/** Task function for resurrector. Do not run directly. */
static void *resurrector_taskfunc(void *arg);

/** Callback function for handling a requerst to checkpoint a thread. */
static void __resurrector_checkpoint(struct crthread *thread);

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/

static void __resurrector_checkpoint(struct crthread *thread)
{
    struct checkpoint *checkpoint;

    /* First, join with the checkpointing thread, which should have exited. */
    pthread_join(thread->ptid, NULL);

    /* Remove the thread from the thread table (PTID is about to change). */
    assert(vtsthreadtable_remove(thread->ptid) == thread);

    checkpoint = thread->checkpoint;
    checkpoint_commit(checkpoint);
    checkpoint_delete(checkpoint);

    /* Finally, simply restore the thread. This should implicitly restore the
     * transient members we destroyed. */
    crthread_restore(thread, false);
}

static void *resurrector_taskfunc(__attribute__((unused))void *arg)
{
    struct vtslist_elem *vtselem;
    struct resurrector_msg *msg;
    bool running = true;

    while (running)
    {
        vtselem = vtslist_pop_front(&self->input);
        msg = container_of(vtselem, struct resurrector_msg, vtselem);

        switch (msg->cmd)
        {
            case RESURRECTOR_SHUTDOWN:
                running = false;
                break;

            case RESURRECTOR_CHECKPOINT:
                __resurrector_checkpoint(msg->thread);
                break;
            
            default:
                abort();
        }

        mc_free(msg);
    }

    return NULL;
}

/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/
void resurrector_init()
{
    vtslist_init(&self->input);
    pthread_create(&self->resworker, NULL, resurrector_taskfunc, NULL);
}

void resurrector_shutdown()
{
    struct resurrector_msg *msg;

    msg = mc_malloc(sizeof(*msg));
    msg->cmd = RESURRECTOR_SHUTDOWN;
    vtslist_push_back(&self->input, &msg->vtselem);

    pthread_join(self->resworker, NULL);
    vtslist_cleanup(&self->input);
}

void resurrector_checkpoint(struct crthread *thread)
{
    struct resurrector_msg *msg;

    msg = mc_malloc(sizeof(*msg));

    msg->cmd = RESURRECTOR_CHECKPOINT;
    msg->thread = thread;
    vtslist_push_back(&self->input, &msg->vtselem);
}
