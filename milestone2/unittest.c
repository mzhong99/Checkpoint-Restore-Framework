#include "unittest.h"
#include <stdio.h>

#define BUFSIZE     256

void run_test(const char *(*test)(), const char *name, const char *description)
{
    const char *message;
    char buffer[BUFSIZE];
    int nprint, ndot;

    printf(ANSI_COLOR_YELLOW "[TEST] " ANSI_COLOR_RESET);
    nprint = snprintf(buffer, BUFSIZE - 1, "%s: %s", name, description);
    printf("%s", buffer);
    buffer[nprint] = '\0';

    ndot = 68 - nprint;

    while (ndot --> 0)
        putchar('.');
    
    message = test();

    if (message == NULL)
        printf(ANSI_COLOR_GREEN "PASS!\n" ANSI_COLOR_RESET);
    else
    {
        printf(ANSI_COLOR_RED "FAIL!\n" ANSI_COLOR_RESET);
        printf("    Message: %s\n", message);
    }
}
