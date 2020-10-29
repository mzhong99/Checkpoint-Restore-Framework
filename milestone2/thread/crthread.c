#include "crthread.h"
#include "crheap.h"
#include "nvstore.h"

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/

/* creation and deletion of non-volatile thread handles */
static struct crthread *crthread_new(void *(*taskfunc) (void *), void *arg);
static void crthread_delete(struct crthread *thread);

/* used as a wrapper for restoration hook on subsequent runs */
static void *crthread_taskfunc(void *thread_vp);

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/
static struct crthread *crthread_new(void *(*taskfunc) (void *), void *arg)
{
    struct crthread *crthread;
    struct nvmetadata *meta;

    meta = nvmetadata_instance();

    crthread = crmalloc(sizeof(*crthread));
    crthread->stacksize = DEFAULT_STACKSIZE;
    crthread->stack = crmalloc(crthread->stacksize);

    crthread->taskfunc = taskfunc;
    crthread->arg = arg;

    crthread->firstrun = true;
    crthread->ptid = -1;

    pthread_mutex_lock(&meta->threadlock);
    list_push_back(&meta->threadlist, &crthread->elem);
    pthread_mutex_unlock(&meta->threadlock);

    return crthread;
}

static void crthread_delete(struct crthread *crthread)
{
    struct nvmetadata *meta;

    meta = nvmetadata_instance();

    pthread_mutex_lock(&meta->threadlock);
    list_remove(&crthread->elem);
    pthread_mutex_unlock(&meta->threadlock);

    crfree(crthread->stack);
    crfree(crthread);
}

static void *crthread_taskfunc(void *crthread_vp)
{
    struct crthread *crthread = crthread_vp;
    void *retval;

    if (crthread->firstrun)
    {
        crthread->firstrun = false;
        retval = crthread->taskfunc(crthread->arg);
    }
    else
        longjmp(crthread->checkpoint, 1);

    return retval;
}

/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/
int crpthread_create(crpthread_t *crptid, void *(*routine) (void *), void *arg)
{
    struct crthread *crthread;
    pthread_attr_t attrs;
    int rc;

    void *stack;
    size_t stacksize;

    /* create a new thread handle with appropriate arguments */
    crthread = crthread_new(routine, arg);
    *crptid = &crthread->ptid;

    /* init pthread attrs for stack setup */
    rc = pthread_attr_init(&attrs);
    if (rc != 0)
        goto fail;

    /* sets stack to either default (first run) OR recovery stack (restore) */
    stack = crthread->stack;
    stacksize = crthread->stacksize;

    if (!crthread->firstrun)
    {
        stack = crthread->recoverystack;
        stacksize = DEFAULT_STACKSIZE;
    }

    rc = pthread_attr_setstack(&attrs, stack, stacksize);
    if (rc != 0)
        goto fail;

    return pthread_create(&crthread->ptid, &attrs, crthread_taskfunc, crthread);

fail:
    crthread_delete(crthread);
    return rc;
}

int crpthread_join(crpthread_t thread, void **retval)
{
    struct crthread *crthread;
    int rc;

    crthread = container_of(thread, struct crthread, ptid);
    rc = pthread_join(crthread->ptid, retval);

    if (rc != 0)
        return rc;

    crthread_delete(crthread);
    return rc;
}

