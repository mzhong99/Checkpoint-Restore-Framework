#include "crthread.h"
#include "crheap.h"
#include "nvstore.h"
#include "vtsthreadtable.h"

#include <assert.h>

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/

/** Used as a wrapper for restoration hook on subsequent runs */
static void *crthread_taskfunc(void *thread_vp);

/** Used to restore a stale thread upon program restart */
static void crthread_restore(struct crthread *thread);

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/

static void crthread_restore(struct crthread *thread)
{
    pthread_attr_t attrs;

    pthread_attr_init(&attrs);
    pthread_attr_setstack(&attrs, thread->recoverystack, DEFAULT_STACKSIZE);

    pthread_create(&thread->ptid, &attrs, thread->taskfunc, thread);
    pthread_attr_destroy(&attrs);
}

static void *crthread_taskfunc(void *crthread_vp)
{
    struct crthread *thread = crthread_vp;
    void *retval;

    /* Initialization of Transient Fields                                     */
    /* ---------------------------------------------------------------------- */

    /* Add thread to volatile global thread table. */
    vtsthreadtable_insert(thread);

    /* Create thread checkpoint object, specifying what memory to checkpoint. */
    thread->checkpoint = checkpoint_new();
    checkpoint_add(thread->checkpoint, thread, sizeof(*thread));
    checkpoint_add(thread->checkpoint, thread->stack, thread->stacksize);
    
    if (thread->firstrun)
    {
        /* If this is the thread's first execution, mark a starting checkpoint
         * and then run the task function inside. */
        thread->firstrun = false;
        crthread_checkpoint();
        retval = thread->taskfunc(thread->arg);
    }
    else
    {
        /* Otherwise, allow execution privileges on the proper stack segment
         * and then jump directly to continue exeuciton. */

        /* TODO: use mprotect() to handle execution permissions. */
        longjmp(thread->env, 1);
    }


    /* Destruction of Transient Fields                                        */
    /* ---------------------------------------------------------------------- */

    /* Remove the thread from the volatile global thread table. */
    vtsthreadtable_remove(thread->ptid);

    /* Destroy thread checkpoint object. */
    checkpoint_delete(thread->checkpoint);

    return retval;
}

/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/

/** We restore each cached thread if the thread crashed during execution.     */
void crthread_init_system()
{
    struct nvmetadata *meta;
    struct list_elem *elem;
    struct crthread *thread;

    meta = nvmetadata_instance();

    vtsthreadtable_init();

    pthread_mutex_lock(&meta->threadlock);

    elem = list_begin(&meta->threadlist);
    while (elem != list_end(&meta->threadlist))
    {
        thread = container_of(elem, struct crthread, elem);
        if (thread->inprogress)
            crthread_restore(thread);

        elem = list_next(elem);
    }

    pthread_mutex_unlock(&meta->threadlock);
}

struct crthread *crthread_new(void *(*taskfunc) (void *), size_t stacksize)
{
    struct crthread *crthread;
    struct nvmetadata *meta;

    meta = nvmetadata_instance();

    crthread = crmalloc(sizeof(*crthread));

    crthread->stacksize = DEFAULT_STACKSIZE;
    if (stacksize > DEFAULT_STACKSIZE)
        crthread->stacksize = stacksize & ~(sysconf(_SC_PAGE_SIZE) - 1);

    crthread->stack = crmalloc(crthread->stacksize);

    crthread->taskfunc = taskfunc;

    crthread->firstrun = true;
    crthread->ptid = -1;

    pthread_mutex_lock(&meta->threadlock);
    list_push_back(&meta->threadlist, &crthread->elem);
    pthread_mutex_unlock(&meta->threadlock);

    nvmetadata_checkpoint(meta);

    return crthread;
}

void crthread_delete(struct crthread *crthread)
{
    struct nvmetadata *meta;

    meta = nvmetadata_instance();

    pthread_mutex_lock(&meta->threadlock);
    list_remove(&crthread->elem);
    pthread_mutex_unlock(&meta->threadlock);

    crfree(crthread->stack);
    crfree(crthread);
}

void crthread_fork(struct crthread *thread, void *arg)
{
    pthread_attr_t attrs;

    thread->arg = arg;

    pthread_attr_init(&attrs);
    pthread_attr_setstack(&attrs, thread->stack, thread->stacksize);

    pthread_create(&thread->ptid, &attrs, crthread_taskfunc, thread);
    pthread_attr_destroy(&attrs);
}

void *crthread_join(struct crthread *thread)
{
    void *retval;

    pthread_join(thread->ptid, &retval);
    thread->inprogress = false;

    return retval;
}

/**
 * The inner process used to checkpoint a thread follows the below algorithm:
 * 
 *  1.) Retrieve the thread handle based on the current pthread_t handle from 
 *      the global thread table.
 * 
 *  2.) Perform a [setjmp()]. The return code of this function will determine 
 *      whether the function is starting or if it is restarting. 
 * 
 *      A return code of 0 implies that the jump buffer was CREATED, meaning 
 *      that we're in the process of creating a checkpoint.
 * 
 *      A return code of 1 implies that the thread jumped to this location. This
 *      implies that we are attempting to RESTORE the program execution context.
 * 
 *  3.) If a checkpoint is requested, we commit the calling thread's current
 *      stack and other miscellaneous thread members.
 */
void crthread_checkpoint()
{
    struct crthread *thread = NULL;
    pthread_t ptid;
    int rc;

    ptid = pthread_self();
    thread = vtsthreadtable_find(ptid);
    assert(thread != NULL);

    rc = setjmp(thread->env);
    if (rc == 0)
        checkpoint_commit(thread->checkpoint);
}
