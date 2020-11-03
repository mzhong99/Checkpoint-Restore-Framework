#include "fibonacci.h"
#include <stdint.h>
#include <stdio.h>

void *fibonacci_tf_serial(void *n_vp)
{
    intptr_t p1, p2, n;

    n = (intptr_t)n_vp;

    if (n == 1 || n == 0)
        return (void *)n;

    p1 = (intptr_t)fibonacci_tf_serial((void *)(n - 1));
    p2 = (intptr_t)fibonacci_tf_serial((void *)(n - 2));

    return (void *)(p1 + p2);
}