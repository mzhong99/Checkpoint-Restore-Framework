#ifndef __CRPTHREAD_H__
#define __CRPTHREAD_H__

#include "list.h"
#include "vtslist.h"
#include "checkpoint.h"

#include <pthread.h>
#include <setjmp.h>
#include <limits.h>

#include <unistd.h>
#include <signal.h>

#define DEFAULT_STACKSIZE   (16 * PTHREAD_STACK_MIN)

/* internal crthread handle - used to represent a non-volatile thread */
struct crthread
{
    /* overhead variables for checkpoint-restore system                       */
    /* ---------------------------------------------------------------------- */
    struct list_elem elem;          /* used for insertions into threadlist    */
    jmp_buf env;                    /* save regs for checkpoint using setjmp  */
    bool firstrun;                  /* skip restoration on first run          */
    bool inprogress;                /* restore if execution was in progress   */

    /* normal thread variables                                                */
    /* ---------------------------------------------------------------------- */
    void *stack;                                 /* crmalloc'd stack          */
    uint8_t recoverystack[DEFAULT_STACKSIZE];    /* recovery stack to longjmp */
    size_t stacksize;                            /* size of main stack        */
    void *(*taskfunc) (void *);                  /* task function             */
    void *arg;                                   /* the argument for thread   */
    void *retval;                                /* result of execution       */

    /* transient fields - values must be restored after a crash with syscalls */
    /* ---------------------------------------------------------------------- */
    pthread_t ptid;                 /* the original thread ID variable        */
    struct vtslist_elem vtselem;    /* used for insertions into vthreadtable  */
    struct checkpoint *checkpoint;  /* checkpoint object for committing self  */
};

/** 
 * Initializes the threading system. Threads can only be created and destroyed
 * after the backing system has been initialized.
 * 
 * Users should not call this function - rather, the framework itself uses this
 * function as part of its calls.
 */
void crthread_init_system();

/** 
 * Shuts down the thread spawning system. We assume that all threads have 
 * already been joined prior to shutting down this system.
 */
void crthread_shutdown_system();

/** 
 * Constructs a new thread. Unlike the pthread library, creating a new thread
 * does not instantly fork off the new function to be executed concurrently.
 */
struct crthread *crthread_new(void *(*taskfunc) (void *), size_t stacksize);

/** 
 * Deletes a thread. The thread should already be joined before deletion. 
 * 
 * Users should call this function.
 */
void crthread_delete(struct crthread *thread);

/** 
 * Forks the thread so that it starts background execution. Threads should be
 * forked only once - subsequent calls to this function will do nothing.
 * 
 * Users should call this function.
 */
void crthread_fork(struct crthread *thread, void *arg);

/** 
 * Causes the calling thread to wait until the supplied thread has finished 
 * execution. The result of the calling thread's function is returned to the 
 * caller.
 * 
 * Users should call this function.
 */
void *crthread_join(struct crthread *thread);

/**
 * Checkpoints the calling thread. This should never be called in the main 
 * thread. This operation only blocks and checkpoints the state of the calling
 * thread - other threads must manually call this function for them to also be
 * checkpointed.
 * 
 * Users can call this function.
 */
void crthread_checkpoint();


#endif
