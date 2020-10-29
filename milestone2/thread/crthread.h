#ifndef __CRPTHREAD_H__
#define __CRPTHREAD_H__

#include "list.h"
#include "vtslist.h"

#include <pthread.h>
#include <setjmp.h>
#include <limits.h>

#define DEFAULT_STACKSIZE   (PTHREAD_STACK_MIN + 0x8000)

/* internal crthread handle - used to represent a non-volatile thread */
struct crthread
{
    /* overhead variables for checkpoint-restore system                       */
    /* ---------------------------------------------------------------------- */
    struct list_elem elem;          /* used for insertions into threadlist    */
    struct vtslist_elem vtselem;    /* used for insertions into vthreadtable  */
    jmp_buf checkpoint;             /* save regs for checkpoint using setjmp  */
    bool firstrun;                  /* skip restoration on first run          */

    /* normal thread variables                                                */
    /* ---------------------------------------------------------------------- */
    void *stack;                                 /* crmalloc'd stack          */
    uint8_t recoverystack[DEFAULT_STACKSIZE];    /* recovery stack to longjmp */
    size_t stacksize;                            /* size of main stack        */
    void *(*taskfunc) (void *);     /* task / thread function                 */
    void *arg;                      /* input arg - MUST be from crmalloc()    */

    /* transient fields - values must be restored after a crash with syscalls */
    /* ---------------------------------------------------------------------- */
    pthread_t ptid;                 /* the original thread ID variable        */
};

typedef pthread_t *crpthread_t;
typedef pthread_attr_t crpthread_attr_t;

int crpthread_create(crpthread_t *thread, void *(*routine) (void *), void *arg);
int crpthread_join(crpthread_t thread, void **retval);

#endif
