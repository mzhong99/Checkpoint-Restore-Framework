#include "unittest.h"
#include <stdio.h>

#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_YELLOW   "\x1b[33m"
#define ANSI_COLOR_BLUE     "\x1b[34m"
#define ANSI_COLOR_MAGENTA  "\x1b[35m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_RESET    "\x1b[0m"

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

    ndot = 62 - nprint;

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
