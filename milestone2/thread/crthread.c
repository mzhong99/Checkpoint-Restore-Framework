#include "crthread.h"
#include "crheap.h"
#include "nvstore.h"
#include "vtsthreadtable.h"

#include <assert.h>

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/

/** 
 * Used to register the stack to be checkpointed. Since stacks grow downwards,
 * and since we believe attempting to checkpoint any stack frame mid-execution
 * causes a hard-to-trace crash which even crashes GDB, we need to separate the
 * main execution stack from the guard.
 * 
 * Do not checkpoint the stack directly. Use this function instead.
*/
static void crthread_add_stack_to_checkpoint(struct crthread *thread, 
                                             struct checkpoint *checkpoint);

/** Used as a wrapper for restoration hook on subsequent runs */
static void *crthread_taskfunc(void *thread_vp);

/** Used to restore a stale thread upon program restart */
static void crthread_restore(struct crthread *thread);

/** Used to recursively descend the stack until the interrupt guard is found  */
static void crthread_descend_and_checkpoint(struct crthread *thread);

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/
static void crthread_add_stack_to_checkpoint(struct crthread *thread, 
                                             struct checkpoint *checkpoint)
{
    size_t user_stacksize;
    void *user_stacktop;

    user_stacksize = thread->stacksize - INTERRUPT_GUARD_SIZE;
    user_stacktop = (uint8_t *)thread->stack + INTERRUPT_GUARD_SIZE;

    checkpoint_add(checkpoint, user_stacktop, user_stacksize);
}

static void crthread_descend_and_checkpoint(struct crthread *thread)
{
    /* Check where we currently are located on the thread stack. */
    volatile void *stackprobe = &thread;

    /* First, we ensure that we have never overflown the stack by checking that
     * we have yet to pass the guard. */
    assert(stackprobe > thread->stack);

    /* Next, if we're still too high in the stack (not enough function calls) 
     * then we perform a recursive descent and skip the checkpoint. */
    if (stackprobe > thread->stack + INTERRUPT_GUARD_SIZE)
    {
        crthread_descend_and_checkpoint(thread);
        return;
    }

    /* Finally, once we're actually deep enough in the stack, we can checkpoint
     * for real. We should have surpassed all user space for our stack. */
    checkpoint_commit(thread->checkpoint);

    /* Once the checkpoint is finished, simply longjmp back to the moment when
     * this thread started descending the stack by piercing through all 
     * recursive calls, and continue execution as normal. */
    longjmp(thread->env, 1);

    /* The longjmp should work - we should never reach this point. */
    assert(false);
}

static void crthread_restore(struct crthread *thread)
{
    pthread_attr_t attrs;

    pthread_attr_init(&attrs);
    pthread_attr_setstack(&attrs, thread->recoverystack, DEFAULT_STACKSIZE);
    pthread_attr_setguardsize(&attrs, 0);

    pthread_create(&thread->ptid, &attrs, thread->taskfunc, thread);
    pthread_attr_destroy(&attrs);
}

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
    crthread_add_stack_to_checkpoint(thread, thread->checkpoint);
    
    if (thread->firstrun)
    {
        /* If this is the thread's first execution, mark a starting checkpoint
         * and then run the task function inside. */
        thread->firstrun = false;
        // crthread_checkpoint();
        thread->retval = thread->taskfunc(thread->arg);
        // crthread_checkpoint();
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

    return thread->retval;
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

void crthread_shutdown_system()
{
    vtsthreadtable_cleanup();
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

    // crthread->stack = crmalloc(crthread->stacksize);
    crthread->stack = nvstore_allocpage(crthread->stacksize / sysconf(_SC_PAGE_SIZE));

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

    // crfree(crthread->stack);
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

    /* Set a marker to which to jump. Then, descend to guard and checkpoint. */
    rc = setjmp(thread->env);
    if (rc == 0)
        crthread_descend_and_checkpoint(thread);
}
