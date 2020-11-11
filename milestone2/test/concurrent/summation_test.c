#include "unittest.h"
#include "memcheck.h"

#include "summation.h"

#include "crheap.h"
#include "checkpoint.h"
#include "crthread.h"

#include <stdio.h>
#include <stdlib.h>

/******************************************************************************/
/** Forward Declarations for Testing Layout --------------------------------- */
/******************************************************************************/

#define SUMMATION_NUM_ELEMENTS  (1 << 19)

/** Callback Functions */
static void summation_ct_setup(void **argp);
static void summation_ct_test(void *arg_vp);
static const char *summation_ct_check_answer(void *arg_vp);

/** The struct used for testing, set as a global variable */
struct checkpoint_tester summation_test = 
{
    .aux = NULL,
    .setup = summation_ct_setup,
    .test = summation_ct_test,
    .check_answer = summation_ct_check_answer,
    .num_allowed_crashes = CRASH_FOREVER,
    .crash_length_us = CRASH_INTERVAL
};

/******************************************************************************/
/** Private Implementation -------------------------------------------------- */
/******************************************************************************/
static void summation_ct_setup(void **argp)
{
    struct summation_args *arg;
    struct checkpoint *checkpoint;

    size_t i;

    crheap_init("summation_test.heap");

    arg = crmalloc(sizeof(*arg));

    arg->len = SUMMATION_NUM_ELEMENTS;
    arg->arr = crmalloc(arg->len * sizeof(*arg->arr));

    for (i = 0; i < arg->len; i++)
        arg->arr[i] = abs(rand()) % 1000;

    *argp = (void *)arg;

    checkpoint = checkpoint_new();
    checkpoint_add(checkpoint, arg, sizeof(*arg));
    checkpoint_add(checkpoint, arg->arr, sizeof(*arg->arr) * arg->len);
    checkpoint_commit(checkpoint);
    checkpoint_delete(checkpoint);

    crheap_shutdown_nosave();
}

static void summation_ct_test(void *arg_vp)
{
    struct crthread *thread = NULL;
    crheap_init("summation_test.heap");

    if (thread == NULL)
    {
        thread = crthread_new(summation_tf_checkpointed, arg_vp, 0);
        crthread_fork(thread);
    }

    if (crheap_get_last_progress() != NV_COMPLETED)
        crthread_join(thread);

    crthread_delete(thread);
    crheap_shutdown();
}

static const char *summation_ct_check_answer(void *arg_vp)
{
    struct summation_args *arg = arg_vp;
    intptr_t reference;

    crheap_init("summation_test.heap");

    reference = summation_serial(arg->arr, arg->len);

    printf("ref=%ld, ans=%ld\n", reference, arg->answer);
    if (reference != arg->answer)
        return "Result did not match.";
    
    crfree(arg->arr);
    crfree(arg);

    crheap_shutdown();

    return NULL;
}