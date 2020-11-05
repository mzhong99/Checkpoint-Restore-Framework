#include "crthread.h"
#include "crheap.h"
#include "nvstore.h"
#include "vtsthreadtable.h"
#include "resurrector.h"
#include "contextswitch.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/

/** Used as a wrapper for restoration hook on subsequent runs */
static void *crthread_stub(void *thread_vp);

/** 
 * Used to force the thread to descend in its stack before executing. The 
 * [callstk] parameter MUST point to a variable on the same stack in which the
 * thread is executing - i.e. the variable's pointer must be located in the
 * memory region specified by [thread->stack].
 */
static void crthread_descend_and_run(struct crthread *thread, void *callstk);

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/

static void crthread_descend_and_run(struct crthread *thread, void *callstk)
{
    long descension;

    /* First, recursively call if we're not far enough into the stack. */
    descension = (long)((intptr_t)callstk - (intptr_t)&callstk);
    if (descension < PTHREAD_STACK_MIN)
        crthread_descend_and_run(thread, callstk);
    else
    {
        /* TODO: Checkpoint BEFORE AND AFTER, KEEP CONTEXTS IN MIND */
        crthread_checkpoint();
        thread->retval = thread->taskfunc(thread->arg);
        crthread_checkpoint();
        load_context(&thread->exitpoint);
    }
    
}

static void *crthread_stub(void *crthread_vp)
{
    struct crthread *thread = crthread_vp;  /* Cast pointer to thread         */
    void *callstk = &crthread_vp;           /* Need a pointer to a stack var. */

    /* Initialization of Transient Fields                                     */
    /* ---------------------------------------------------------------------- */

    /* Add thread to volatile global thread table. */
    vtsthreadtable_insert(thread);

    /* Create thread checkpoint object, specifying what memory to checkpoint. */
    thread->checkpoint = checkpoint_new();
    checkpoint_add(thread->checkpoint, thread, sizeof(*thread));
    checkpoint_add(thread->checkpoint, thread->stack, thread->stacksize);

    /* EXIT POINT 1: 
     * The thread exited normally. */
    if (save_context(&thread->exitpoint) != 0)
    {
        /* Remove the thread from the volatile global thread table. */
        vtsthreadtable_remove(thread->ptid);

        /* Destroy thread checkpoint object. */
        checkpoint_delete(thread->checkpoint);

        /* Post to calling thread that execution is complete. */
        sem_post(thread->userjoin);

        /* Finally, exit for real. */
        pthread_exit(NULL);
    }

    /* EXIT POINT 2:
     * The thread exited prematurely to prepare for a checkpoint. */
    if (save_context(&thread->cpexitpoint) != 0)
    {
        /* Simply exit for real. Do NOT post to userjoin and leave transient 
         * fields alone. */
        pthread_exit(NULL);
    }

    if (thread->firstrun)
    {
        thread->firstrun = false;

        /* If this is the thread's first execution, recursively descend the 
         * and then execute the thread's function */
        crthread_descend_and_run(thread, callstk);
    }
    else
    {
        /* Otherwise, load the thread's restore point. */
        load_context(&thread->restorepoint);
    }

    /* Function should never reach this point. */
    abort();
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
    crthread->userjoin = mcmalloc(sizeof(*crthread->userjoin));
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

    sem_destroy(crthread->userjoin);
    mcfree(crthread->userjoin);
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

    pthread_create(&thread->ptid, &attrs, crthread_stub, thread);
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
    struct crthread *volatile thread = NULL;
    struct crcontext *volatile context;

    pthread_t ptid;

    ptid = pthread_self();
    thread = vtsthreadtable_find(ptid);

    assert(thread != NULL);
    context = (struct crcontext *volatile)&thread->restorepoint;

    /* Set a marker to which to jump. Then, submit this thread to the 
     * resurrector and exit for restoration. */
    if (save_context(context) == 0)
    {
        // display_context(context);
        resurrector_checkpoint(thread);

        /* Jump to thread's checkpoint exit location. */
        load_context(&thread->cpexitpoint);
    }

    // display_context(context);
}

void crthread_restore(struct crthread *thread, bool from_file)
{
    pthread_attr_t attrs;

    pthread_attr_init(&attrs);
    pthread_attr_setstack(&attrs, thread->recoverystack, DEFAULT_STACKSIZE);
    pthread_attr_setguardsize(&attrs, 0);

    if (from_file)
    {
        thread->userjoin = mcmalloc(sizeof(*thread->userjoin));
        sem_init(thread->userjoin, 0, 0);
    }

    pthread_create(&thread->ptid, &attrs, crthread_stub, thread);
    pthread_attr_destroy(&attrs);
}
