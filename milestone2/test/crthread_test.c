#include "crthread_test.h"
#include "crheap.h"
#include "crthread.h"
#include "fibonacci.h"

#include <stddef.h>
#include <stdint.h>

const char *test_crthread_basic()
{
    struct crthread *thread;
    intptr_t result;

    crheap_init("test_crthread_basic.heap");

    thread = crthread_new(fibonacci_tf_serial, 0);
    crthread_fork(thread, (void *)9);
    result = (intptr_t)crthread_join(thread);
    crthread_delete(thread);

    crheap_shutdown();

    if (result != 34)
        return "Fibonacci result was not correct.";
    return NULL;
}
