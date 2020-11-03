#include "nvstore_test.h"
#include "nvstore.h"
#include "memcheck.h"

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
    refdata = mc_malloc(sysconf(_SC_PAGE_SIZE) * SMALL_NUM_PAGES);

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

    mc_free(refdata);
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
        refdata = mc_malloc(sysconf(_SC_PAGE_SIZE) * (i + 1));
        data = nvstore_allocpage(i + 1);

        for (j = 0; j < sysconf(_SC_PAGE_SIZE) * (i + 1); j++)
        {
            refdata[j] = (uint8_t)rand();
            data[j] = refdata[j];
        }

        for (j = 0; j < sysconf(_SC_PAGE_SIZE) * (i + 1); j++)
            if (data[j] != refdata[j])
                return "Data contents do not match";

        mc_free(refdata);
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

    refdata = mc_malloc(sysconf(_SC_PAGE_SIZE));
    data = nvstore_allocpage(1);
    
    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
    {
        refdata[i] = 0x55;
        data[i] = refdata[i];
    }

    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
        if (data[i] != refdata[i])
            return "Contents do not match prior to restoration.";

    nvstore_checkpoint_everything();

    rc = nvstore_shutdown();
    if (rc != 0)
        return "First shutdown failed.";

    rc = nvstore_init("test_nvstore_checkpoint_simple.heap");
    if (rc != 0)
        return "Second initialization failed.";

    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
        if (data[i] != refdata[i])
            return "Contents do not match after restoration.";

    rc = nvstore_shutdown();
    if (rc != 0)
        return "Second shutdown failed.";

    mc_free(refdata);
    return NULL;
}

const char *test_nvstore_checkpoint_complex()
{
    uint8_t *datamany[LARGE_NUM_PAGES], *refdatamany[LARGE_NUM_PAGES];
    int i, j, k, rc;

    for (i = 0; i < LARGE_NUM_PAGES; i++)
    {
        rc = nvstore_init("test_nvstore_checkpoint_complex.heap");
        if (rc != 0)
            return "First initialization failed.";

        refdatamany[i] = mc_malloc((i + 1) * sysconf(_SC_PAGE_SIZE));
        datamany[i] = nvstore_allocpage(i + 1);

        for (j = 0; j <= i; j++)
        {
            for (k = 0; k < (j + 1) * sysconf(_SC_PAGE_SIZE); k++)
            {
                refdatamany[j][k] = (uint8_t)rand();
                datamany[j][k] = refdatamany[j][k];
            }
        }
        
        for (j = 0; j <= i; j++)
            for (k = 0; k < (j + 1) * sysconf(_SC_PAGE_SIZE); k++)
                if (refdatamany[j][k] != datamany[j][k])
                    return "Contents do not match prior to restoration.";

        nvstore_checkpoint_everything();

        rc = nvstore_shutdown();
        if (rc != 0)
            return "First shutdown failed";

        rc = nvstore_init("test_nvstore_checkpoint_complex.heap");
        if (rc != 0)
            return "Second initialization failed.";

        for (j = 0; j <= i; j++)
            for (k = 0; k < (j + 1) * sysconf(_SC_PAGE_SIZE); k++)
                if (refdatamany[j][k] != datamany[j][k])
                    return "Contents do not match after restoration.";

        rc = nvstore_shutdown();
        if (rc != 0)
            return "Second shutdown failed";
    }

    for (i = 0; i < LARGE_NUM_PAGES; i++)
        mc_free(refdatamany[i]);

    return NULL;
}

const char *test_nvstore_checkpoint_without_shutdown()
{
    uint8_t *datamany[LARGE_NUM_PAGES], *refdatamany[LARGE_NUM_PAGES];
    int i, j, k, rc;

    for (i = 0; i < LARGE_NUM_PAGES; i++)
    {
        rc = nvstore_init("test_nvstore_checkpoint_without_shutdown.heap");
        if (rc != 0)
            return "First initialization failed.";

        refdatamany[i] = mc_malloc((i + 1) * sysconf(_SC_PAGE_SIZE));
        datamany[i] = nvstore_allocpage(i + 1);

        for (j = 0; j <= i; j++)
        {
            for (k = 0; k < (j + 1) * sysconf(_SC_PAGE_SIZE); k++)
            {
                refdatamany[j][k] = (uint8_t)rand();
                datamany[j][k] = refdatamany[j][k];
            }
        }
        
        for (j = 0; j <= i; j++)
            for (k = 0; k < (j + 1) * sysconf(_SC_PAGE_SIZE); k++)
                if (refdatamany[j][k] != datamany[j][k])
                    return "Contents do not match prior to restoration.";

        nvstore_checkpoint_everything();

        for (j = 0; j <= i; j++)
            for (k = 0; k < (j + 1) * sysconf(_SC_PAGE_SIZE); k++)
                if (refdatamany[j][k] != datamany[j][k])
                    return "Contents do not match after first checkpoint but "
                           "before first shutdown.";

        rc = nvstore_shutdown();
        if (rc != 0)
            return "First shutdown failed";

        rc = nvstore_init("test_nvstore_checkpoint_without_shutdown.heap");
        if (rc != 0)
            return "Second initialization failed.";

        for (j = 0; j <= i; j++)
            for (k = 0; k < (j + 1) * sysconf(_SC_PAGE_SIZE); k++)
                if (refdatamany[j][k] != datamany[j][k])
                    return "Contents do not match after restoration.";

        nvstore_checkpoint_everything();

        for (j = 0; j <= i; j++)
            for (k = 0; k < (j + 1) * sysconf(_SC_PAGE_SIZE); k++)
                if (refdatamany[j][k] != datamany[j][k])
                    return "Contents do not match after restart + checkpoint "
                           "but before shutdown.";

        rc = nvstore_shutdown();
        if (rc != 0)
            return "Second shutdown failed";
    }

    for (i = 0; i < LARGE_NUM_PAGES; i++)
        mc_free(refdatamany[i]);

    return NULL;
}