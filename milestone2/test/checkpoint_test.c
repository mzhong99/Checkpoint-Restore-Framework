#include "checkpoint_test.h"
#include "crheap.h"

#include "checkpoint.h"
#include "memcheck.h"

#include <stdlib.h>

#define TEST_SIZE   30000

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

const char *test_checkpoint_stack()
{

    return NULL;
}