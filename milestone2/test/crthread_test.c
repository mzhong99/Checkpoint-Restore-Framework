#include "crthread_test.h"
#include "crheap.h"
#include "crthread.h"
#include "fibonacci.h"
#include "summation.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FIBONACCI_DEPTH_EASY    36
#define FIBONACCI_DEPTH_HARD    39

#define NUM_ELEMENTS            (1 << 21)

const char *test_crthread_basic()
{
    struct crthread *thread;
    intptr_t result, reference;

    reference = fibonacci_fast(FIBONACCI_DEPTH_EASY);

    crheap_init("test_crthread_basic.heap");

    thread = crthread_new(fibonacci_tf_serial, (void *)FIBONACCI_DEPTH_EASY, 0);
    crthread_fork(thread);
    result = (intptr_t)crthread_join(thread);
    crthread_delete(thread);

    crheap_shutdown();

    if (result != reference)
        return "Fibonacci result was not correct.";
    return NULL;
}

const char *test_crthread_restore_graceful()
{
    struct crthread *thread;
    pid_t child;
    intptr_t result, reference;

    /* First, compute the actual answer of the test. */
    reference = (intptr_t)fibonacci_tf_serial((void *)FIBONACCI_DEPTH_HARD);

    /* Before spawning a child to kill, first, we initialize the thread. */
    crheap_init("test_crthread_restore_graceful.heap");
    thread = crthread_new(fibonacci_tf_serial, (void *)FIBONACCI_DEPTH_HARD, 0);
    crheap_shutdown();

    /* Fork off a child to test non-volatility. */
    child = fork();
    if (child == -1)
        return "System fork failed.";
    
    if (child == 0)
    {
        /* As the child, attempt to start the testing routine. */
        crheap_init("test_crthread_restore_graceful.heap");
        crthread_fork(thread);
        crheap_shutdown_nosave();

        exit(EXIT_SUCCESS);
    }
    else
    {
        /* As the parent, wait for the child to run for a bit, then kill it. */
        usleep(50000);
        kill(child, SIGKILL);

        /* Once the child is killed, attempt a system restore. The thread should
         * already be runnable, meaning you can join with it. */
        crheap_init("test_crthread_restore_graceful.heap");

        result = (intptr_t)crthread_join(thread);
        crthread_delete(thread);

        crheap_shutdown();

        /* Check that the computational result is correct. */
        if (result != reference)
            return "Fibonacci result was not correct.";
    }
    
    return NULL;
}