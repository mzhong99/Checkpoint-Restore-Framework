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

    /* normal thread variables                                                */
    /* ---------------------------------------------------------------------- */
    void *stack;                    /* crmalloc'd stack                       */
    size_t stacksize;               /* size of requested stack                */

    void *(*taskfunc) (void *);     /* task / thread function                 */
    void *arg;                      /* input arg - MUST be from crmalloc()    */
    void *retval;                   /* return value from thread execution     */

    crpthread_t ptid;               /* the original thread ID variable        */
    crpthread_attr_t attr;          /* attributes for respawning the thread   */
};

static struct crthread *crthread_new(const crpthread_attr_t *attr, 
                                     void *(*taskfunc) (void *), void *arg);
static void crthread_delete(struct crthread *thread);

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/
static struct crthread *crthread_new(const crpthread_attr_t *attr, 
                                     void *(*taskfunc) (void *), void *arg)
{
    struct crthread *thread;
    struct nvmetadata *meta;

    meta = nvmetadata_instance();

    thread = crmalloc(sizeof(*thread));
    thread->stacksize = DEFAULT_STACKSIZE;

    thread->taskfunc = taskfunc;
    thread->arg = arg;
    thread->retval = NULL;

    thread->ptid = -1;
    if (attr != NULL)
        thread->attr = *attr;

    pthread_mutex_lock(&meta->threadlock);
    list_push_back(&meta->threadlist, &thread->elem);
    pthread_mutex_unlock(&meta->threadlock);

    return thread;
}

static void crthread_delete(struct crthread *thread)
{
    crfree(thread->stack);
    crfree(thread);
}

/******************************************************************************/
/** Public-Facing API ------------------------------------------------------- */
/******************************************************************************/
int crpthread_create(crpthread_t *crptid, const crpthread_attr_t *attr,
                     void *(*start_routine) (void *), void *arg)
{
    struct crthread *thread;

    thread = crthread_new(attr, start_routine, arg);

    return 0;
}
