#include "nvstore_test.h"
#include "nvstore.h"

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define SMALL_NUM_PAGES     4
#define LARGE_NUM_PAGES     16

const char *test_nvstore_init()
{
    int rc;

    rc = nvstore_init("test_nvstore_init.heap");
    if (rc != 0)
        return "Initialization failed.";

    nvstore_shutdown();
    if (rc != 0)
        return "Shutdown failed.";

    return NULL;
}

const char *test_nvstore_alloc_simple()
{
    uint8_t *data, *refdata;
    int i, rc;

    rc = nvstore_init("test_nvstore_alloc_simple.heap");
    if (rc != 0)
        return "Initialization failed.";

    data = nvstore_allocpage(SMALL_NUM_PAGES);
    refdata = malloc(sysconf(_SC_PAGE_SIZE) * SMALL_NUM_PAGES);

    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
    {
        refdata[i] = (uint8_t)rand();
        data[i] = refdata[i];
    }

    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
        if (data[i] != refdata[i])
            return "Data contents do not match";

    rc = nvstore_shutdown();
    if (rc != 0)
        return "Shutdown failed.";

    free(refdata);
    return NULL;
}

const char *test_nvstore_alloc_complex()
{
    uint8_t *data, *refdata;
    int i, j, rc;

    rc = nvstore_init("test_nvstore_alloc_complex.heap");
    if (rc != 0)
        return "Initialization failed.";

    for (i = 0; i < LARGE_NUM_PAGES; i++)
    {
        refdata = malloc(sysconf(_SC_PAGE_SIZE) * (i + 1));
        data = nvstore_allocpage(i + 1);

        for (j = 0; j < sysconf(_SC_PAGE_SIZE) * (i + 1); j++)
        {
            refdata[j] = (uint8_t)rand();
            data[j] = refdata[j];
        }

        for (j = 0; j < sysconf(_SC_PAGE_SIZE) * (i + 1); j++)
            if (data[j] != refdata[j])
                return "Data contents do not match";

        free(refdata);
    }

    rc = nvstore_shutdown();
    if (rc != 0)
        return "Shutdown failed.";

    return NULL;
}

const char *test_nvstore_checkpoint_simple()
{
    uint8_t *data, *refdata;
    int i, rc;

    rc = nvstore_init("test_nvstore_checkpoint_simple.heap");
    if (rc != 0)
        return "First initialization failed.";

    refdata = malloc(sysconf(_SC_PAGE_SIZE));
    data = nvstore_allocpage(1);
    
    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
    {
        refdata[i] = 0x55;
        data[i] = refdata[i];
    }

    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
        if (data[i] != refdata[i])
            return "Contents do not match prior to restoration";

    nvstore_checkpoint();

    nvstore_shutdown();
    if (rc != 0)
        return "First shutdown failed.";

    rc = nvstore_init("test_nvstore_checkpoint_simple.heap");
    if (rc != 0)
        return "Second initialization failed.";

    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
        if (data[i] != refdata[i])
            return "Contents do not match after restoration";

    rc = nvstore_shutdown();
    if (rc != 0)
        return "Second shutdown failed.";

    free(refdata);
    return NULL;
}
