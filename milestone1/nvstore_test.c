#include "nvstore_test.h"
#include "nvstore.h"

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#define SMALL_NUM_PAGES     4

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

    nvstore_shutdown();
    if (rc != 0)
        return "Shutdown failed.";

    free(refdata);
    return NULL;
}

const char *test_nvstore_alloc_complex()
{
    return NULL;
}
