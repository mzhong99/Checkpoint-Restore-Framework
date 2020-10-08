#include "unittest.h"
#include <stdio.h>

void run_test(const char *(*test)(), const char *name, const char *description)
{
    const char *message;

    printf("[TEST] %s: %s...", name, description);
    message = test();

    if (message == NULL)
        printf("PASS!\n");
    else
    {
        printf("FAIL!\n");
        printf("    Message: %s\n", message);
    }
}
