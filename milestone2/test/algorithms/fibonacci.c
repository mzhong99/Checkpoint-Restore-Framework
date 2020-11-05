#include "fibonacci.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static int64_t fibonacci_recursive(int64_t n)
{
    int64_t p1, p2;

    if (n == 1 || n == 0)
        return n;

    p1 = fibonacci_recursive(n - 1);
    p2 = fibonacci_recursive(n - 2);

    return p1 + p2;
}

void *fibonacci_tf_serial(void *n_vp)
{
    return (void *)fibonacci_recursive((int64_t)n_vp);
}

intptr_t fibonacci_fast(int n)
{
    int curr, prev, tmp, i;

    if (n <= 1)
        return n;

    curr = 1;
    prev = 1;

    for (i = 2; i < n; i++)
    {
        tmp = curr;
        curr += prev;
        prev = tmp;
    }

    return curr;
}