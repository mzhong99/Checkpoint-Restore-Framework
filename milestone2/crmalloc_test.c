#include "crmalloc_test.h"
#include "crheap.h"

#include <stdlib.h>
#include <unistd.h>

#define NTRIALS 20

const char *test_crmalloc_simple()
{
    int i, *data;

    crheap_init("test_crmalloc_simple.heap");
    data = crmalloc(sizeof(int) * sysconf(_SC_PAGE_SIZE));
    
    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
        data[i] = i;

    for (i = 0; i < sysconf(_SC_PAGE_SIZE); i++)
        if (data[i] != i)
            return "Data written to a crmalloc() block is incorrect";

    crfree(data);
    crheap_shutdown();

    return NULL;
}

const char *test_crmalloc_complex()
{
    int i, trial, npages, *data;

    crheap_init("test_crmalloc_complex.heap");

    for (trial = 0; trial < NTRIALS; trial++)
    {
        npages = (abs(rand()) % (trial + 1)) + 1;
        data = crmalloc(sizeof(int) * sysconf(_SC_PAGE_SIZE) * npages);

        for (i = 0; i < sysconf(_SC_PAGE_SIZE) * npages; i++)
            data[i] = i;

        for (i = 0; i < sysconf(_SC_PAGE_SIZE) * npages; i++)
            if (data[i] != i)
                return "Data written to a crmalloc() block is incorrect";

        crfree(data);
    }

    crheap_shutdown();
    return NULL;
}
