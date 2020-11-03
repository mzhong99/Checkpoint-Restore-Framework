#include "crthread.h"
#include "crheap.h"
#include "nvstore.h"
#include "vtsthreadtable.h"
#include "resurrector.h"

#include <assert.h>

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/

/** Used as a wrapper for restoration hook on subsequent runs */
static void *crthread_taskfunc(void *thread_vp);

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/
static void *crthread_taskfunc(void *crthread_vp)
{
    struct crthread *thread = crthread_vp;

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

        /* Checkpoint BEFORE and AFTER thread execution. */
        crthread_checkpoint();
        thread->retval = thread->taskfunc(thread->arg);
        crthread_checkpoint();
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

    /* Exit this function manually. DO NOT RETURN, in case of recovery. */
    pthread_exit(NULL);

    /* Function should never reach this point. */
    assert(false);
    return NULL;
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
    resurrector_init();

    pthread_mutex_lock(&meta->threadlock);

    elem = list_begin(&meta->threadlist);
    while (elem != list_end(&meta->threadlist))
    {
        thread = container_of(elem, struct crthread, elem);
        if (thread->inprogress)
            crthread_restore(thread, true);

        elem = list_next(elem);
    }

    pthread_mutex_unlock(&meta->threadlock);
}

void crthread_shutdown_system()
{
    vtsthreadtable_cleanup();
    resurrector_shutdown();
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

    /* Semaphore is initialized outside of task function so that main thread can
     * join with this crthread. */
    crthread->userjoin = mc_malloc(sizeof(*crthread->userjoin));
    sem_init(crthread->userjoin, 0, 0);

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
    pthread_attr_setguardsize(&attrs, 0);

    pthread_create(&thread->ptid, &attrs, crthread_taskfunc, thread);
    pthread_attr_destroy(&attrs);
}

void *crthread_join(struct crthread *thread)
{
    sem_wait(thread->userjoin);
    thread->inprogress = false;

    return thread->retval;
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

    /* Set a marker to which to jump. Then, submit this thread to the 
     * resurrector and exit for restoration. */
    rc = setjmp(thread->env);
    if (rc == 0)
    {
        resurrector_checkpoint(thread);
        pthread_exit(NULL);
    }
}

void crthread_restore(struct crthread *thread, bool from_file)
{
    pthread_attr_t attrs;

    pthread_attr_init(&attrs);
    pthread_attr_setstack(&attrs, thread->recoverystack, DEFAULT_STACKSIZE);
    pthread_attr_setguardsize(&attrs, 0);

    if (from_file)
    {
        thread->userjoin = mc_malloc(sizeof(*thread->userjoin));
        sem_init(thread->userjoin, 0, 0);
    }

    pthread_create(&thread->ptid, &attrs, crthread_taskfunc, thread);
    pthread_attr_destroy(&attrs);
}
