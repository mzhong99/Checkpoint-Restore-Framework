#include "crthread_test.h"
#include "crheap.h"
#include "crthread.h"
#include "fibonacci.h"

#include <stddef.h>
#include <stdint.h>

#define FIBONACCI_DEPTH     5

const char *test_crthread_basic()
{
    struct crthread *thread;
    intptr_t result, reference;

    reference = (intptr_t)fibonacci_tf_serial((void *)FIBONACCI_DEPTH);

    crheap_init("test_crthread_basic.heap");

    thread = crthread_new(fibonacci_tf_serial, 0);
    crthread_fork(thread, (void *)FIBONACCI_DEPTH);
    result = (intptr_t)crthread_join(thread);
    crthread_delete(thread);

    crheap_shutdown();

    if (result != reference)
        return "Fibonacci result was not correct.";
    return NULL;
}
