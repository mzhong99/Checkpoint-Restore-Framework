#include "summation.h"
#include "crthread.h"
#include "checkpoint.h"

#include <stdio.h>

#define CHECKPOINT_LIMIT    (1 << 17)

void *summation_tf_checkpointed(void *arg_vp)
{
    struct summation_args *args;
    struct checkpoint *checkpoint;
    
    fprintf(stderr, "STARTING\n");

    args = arg_vp;
    args->answer = summation_checkpointed(args);

    checkpoint = checkpoint_new();
    checkpoint_add(checkpoint, args, sizeof(*args));
    checkpoint_commit(checkpoint);
    checkpoint_delete(checkpoint);

    crthread_checkpoint();

    return NULL;
}

intptr_t summation_checkpointed(struct summation_args *args)
{
    struct checkpoint *checkpoint;

    intptr_t result;
    size_t i;

    for (i = 0; i < args->len; i++)
    {
        result += args->arr[i];
        fprintf(stderr, "result: %ld\n", result);

        if (i % CHECKPOINT_LIMIT == 0)
        {
            checkpoint = checkpoint_new();
            checkpoint_add(checkpoint, args, sizeof(*args));
            checkpoint_commit(checkpoint);
            checkpoint_delete(checkpoint);

            crthread_checkpoint();
        }
    }

    return result;
}

intptr_t summation_serial(intptr_t *arr, size_t len)
{
    intptr_t answer = 0;
    size_t i;

    for (i = 0; i < len; i++)
        answer += arr[i];
    
    return answer;
}