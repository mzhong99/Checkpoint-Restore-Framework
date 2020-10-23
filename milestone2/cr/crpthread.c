#include "crpthread.h"
#include "crheap.h"
#include "nvstore.h"

#include "list.h"

#include <pthread.h>
#include <setjmp.h>
#include <limits.h>

/******************************************************************************/
/** Macros, Definitions, and Static Variables ------------------------------- */
/******************************************************************************/
#define DEFAULT_STACKSIZE   (PTHREAD_STACK_MIN + 0x8000)

/* internal crthread handle - used to represent a non-volatile thread */
struct crthread
{
    /* overhead variables for checkpoint-restore system                       */
    /* ---------------------------------------------------------------------- */
    struct list_elem elem;          /* used for insertions into threadlist    */
    jmp_buf checkpoint;             /* save regs for checkpoint using setjmp  */
    bool firstrun;                  /* skip restoration on first run          */

    /* normal thread variables                                                */
    /* ---------------------------------------------------------------------- */
    void *stack;                    /* crmalloc'd stack                       */
    size_t stacksize;               /* size of requested stack                */

    void *(*taskfunc) (void *);     /* task / thread function                 */
    void *arg;                      /* input arg - MUST be from crmalloc()    */

    /* transient fields - values must be restored after a crash with syscalls */
    /* ---------------------------------------------------------------------- */
    pthread_t ptid;                 /* the original thread ID variable        */
};

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

    crthread = crthread_new(routine, arg);
    *crptid = &crthread->ptid;

    rc = pthread_attr_init(&attrs);
    if (rc != 0)
        goto fail;

    rc = pthread_attr_setstack(&attrs, crthread->stack, crthread->stacksize);
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

