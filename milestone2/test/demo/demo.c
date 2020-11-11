#include "demo.h"
#include "crthread.h"
#include "crheap.h"

#include "fibonacci.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define SIEVE_LIMIT (65536 * 9)

static void *demo_primesieve_tf(void *arg)
{
    size_t i, j, k;
    bool sievearr[SIEVE_LIMIT];

    (void)arg;

    printf("Started sieve for %d elements.\n", SIEVE_LIMIT);

    for (i = 0; i < SIEVE_LIMIT; i++)
        sievearr[i] = true;

    crthread_checkpoint();
    printf("Prepopulation and checkpointing finished.\n");

    for (i = 2; i * i < SIEVE_LIMIT; i++)
    {
        if (!sievearr[i])
            continue;
        
        for (j = 0; j < SIEVE_LIMIT; j++)
        {
            k = (i * i) + (j * i);

            if (k >= SIEVE_LIMIT)
                break;

            sievearr[k] = false;
        }

        printf("Determined N=%ld is prime, checkpointing...", i);
        crthread_checkpoint();
        printf("OK\n");
    }

    return NULL;
}

void demo_primesieve()
{
    struct crthread *thread;

    crheap_init(NULL);

    printf("Starting system...\n");
    if (crheap_get_last_progress() == NV_FIRSTRUN)
    {
        thread = crthread_new(demo_primesieve_tf, NULL, SIEVE_LIMIT * 20);
        crthread_fork(thread);
    }

    crthread_join(thread);
    crheap_shutdown();
}


static void *demo_fibonacci_tf(void *arg)
{
    int64_t N, answer;
    (void)arg;

    N = 1;

    for (;;)
    {
        printf("Beginning fibonacci computation for %ld\n", N);
        answer = fibonacci_recursive(N);
        printf("Computed answer for %ld -> %ld\n", N, answer);

        N++;

        crthread_checkpoint();
    }

    return NULL;
}

void demo_fibonacci()
{
    struct crthread *thread;

    crheap_init(NULL);

    printf("Starting system...\n");
    if (crheap_get_last_progress() == NV_FIRSTRUN)
    {
        thread = crthread_new(demo_fibonacci_tf, NULL, 0);
        crthread_fork(thread);
    }

    // crthread_join(thread);
    for (;;);

    crheap_shutdown();
}