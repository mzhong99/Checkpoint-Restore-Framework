#include "checkpoint_test.h"
#include "crheap.h"

#include "checkpoint.h"
#include "memcheck.h"
#include "contextswitch.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <pthread.h>
#include <unistd.h>

#define TEST_SIZE           20000
#define THREAD_STACKSIZE    1048576

const char *test_checkpoint_basic()
{
    struct checkpoint *checkpoint;
    int *data, *refdata, i;

    crheap_init("test_checkpoint_basic.heap");

    refdata = mcmalloc(sizeof(*refdata) * TEST_SIZE);
    data = crmalloc(sizeof(*data) * TEST_SIZE);

    for (i = 0; i < TEST_SIZE; i++)
        data[i] = refdata[i] = rand();

    for (i = 0; i < TEST_SIZE; i++)
        if (data[i] != refdata[i])
            return "Data does not match upon first generation.";

    checkpoint = checkpoint_new();
    checkpoint_add(checkpoint, data, TEST_SIZE * sizeof(*data));
    checkpoint_commit(checkpoint);

    for (i = 0; i < TEST_SIZE; i++)
        if (data[i] != refdata[i])
            return "Data does not match after first checkpoint but before shutdown.";

    for (i = 0; i < TEST_SIZE; i++)
        data[i] = refdata[i] = rand();

    checkpoint_commit(checkpoint);

    for (i = 0; i < TEST_SIZE; i++)
        if (data[i] != refdata[i])
            return "Data does not match when attempting to reuse checkpoint object.";

    checkpoint_delete(checkpoint);

    crheap_shutdown_nosave();

    crheap_init("test_checkpoint_basic.heap");

    for (i = 0; i < TEST_SIZE; i++)
        if (data[i] != refdata[i])
            return "Data does not match after restoration";
    
    crheap_shutdown_nosave();

    mcfree(refdata);
    return NULL;
}

/******************************************************************************/
/** Thread Checkpointing Basics --------------------------------------------- */
/******************************************************************************/
static void __attribute__((unused)) pthread_descend_and_exit(void *retval) 
{
    long descension;

    descension = (long)((intptr_t)retval - (intptr_t)&retval);
    printf("%ld\n", descension);
    if (descension < 16 * sysconf(_SC_PAGE_SIZE))
        pthread_descend_and_exit(retval);
    else
        pthread_exit(retval);
}

static void *stack_variables_tf(void *arr_vp)
{
    int stackdata[TEST_SIZE];
    int *refdata, i;
    volatile uint64_t helper;

    refdata = arr_vp;
    for (i = 0; i < TEST_SIZE; i++)
        stackdata[i] = refdata[i];

    helper = (volatile uint64_t)&stackdata[0];
    return (void *)helper;
}

static void descend_and_execute(
    void *(func) (void *), void *arg, void **retval, struct crcontext *context)
{
    long descension;
    descension = (long)((intptr_t)retval - (intptr_t)&retval);
    if (descension < 16 * sysconf(_SC_PAGE_SIZE))
        descend_and_execute(func, arg, retval, context);
    else
    {
        *retval = func(arg);
        load_context(context);
    }
}

static void *thread_stub_function(void *arr_vp)
{
    struct crcontext context;
    volatile void *volatile retval;

    if (save_context(&context) == 0)
        descend_and_execute(stack_variables_tf, arr_vp, (void *)&retval, &context);

    return (void *)retval;
}

const char *test_checkpoint_stack_variables()
{
    struct checkpoint *checkpoint;
    int refdata[TEST_SIZE], i, *data;
    void *stack;

    pthread_t thread;
    pthread_attr_t attrs;

    for (i = 0; i < TEST_SIZE; i++)
        refdata[i] = rand();

    crheap_init("test_checkpoint_stack_variables.heap");

    stack = crmalloc(THREAD_STACKSIZE);

    pthread_attr_init(&attrs);
    pthread_attr_setstack(&attrs, stack, THREAD_STACKSIZE);

    pthread_create(&thread, &attrs, thread_stub_function, refdata);
    pthread_attr_destroy(&attrs);
    pthread_join(thread, (void **)&data);

    for (i = 0; i < TEST_SIZE; i++)
        if (data[i] != refdata[i])
            return "Data does not match upon first return.";
    
    checkpoint = checkpoint_new();
    checkpoint_add(checkpoint, stack, THREAD_STACKSIZE);
    checkpoint_commit(checkpoint);
    checkpoint_delete(checkpoint);

    crfree(stack);

    crheap_shutdown_nosave();

    crheap_init("test_checkpoint_stack_variables.heap");

    for (i = 0; i < TEST_SIZE; i++)
        if (data[i] != refdata[i])
            return "Data does not match upon restoration.";

    crheap_shutdown();

    return NULL;
}