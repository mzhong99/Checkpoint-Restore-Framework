#include "unittest.h"
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>

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

    fflush(stdout);
    
    message = test();

    if (message == NULL)
        printf(ANSI_COLOR_GREEN "PASS!" ANSI_COLOR_RESET "\n");
    else
    {
        printf(ANSI_COLOR_RED "FAIL!" ANSI_COLOR_RESET "\n");
        printf("    Message: %s\n", message);
    }
}

void run_concurrent_test(struct checkpoint_tester *tester,
                         const char *name, const char *description)
{
    const char *message;
    char buffer[BUFSIZE];
    int nprint, ndot, status;
    size_t ncrash;

    pid_t child, waitrc;

    printf(ANSI_COLOR_YELLOW "[TEST] " ANSI_COLOR_RESET);
    nprint = snprintf(buffer, BUFSIZE - 1, "%s: %s", name, description);
    printf("%s", buffer);
    buffer[nprint] = '\0';

    ndot = 68 - nprint;

    while (ndot --> 0)
        putchar('.');

    fflush(stdout);

    tester->setup(&tester->aux);

    for (ncrash = 0; ncrash < tester->num_allowed_crashes; ncrash++)
    {
        child = fork();
        assert(child != -1);

        if (child == 0)
        {
            tester->test(&tester->aux);
            exit(EXIT_SUCCESS);
        }
        else
        {
            usleep(tester->crash_length_us);
            waitrc = waitpid(child, &status, WNOHANG);

            /* If execution done, exit crash loop. Otherwise, kill / restart */
            if (waitrc == child)
                break;
            else
            {
                printf("Killing...\n");
                kill(child, SIGKILL);
            }
        }
    }

    if (ncrash >= tester->num_allowed_crashes)
    {
        printf(ANSI_COLOR_RED "FAIL!" ANSI_COLOR_RESET "\n");
        printf("    Message: Test timed out.\n");
    }
    else
    {
        message = tester->check_answer(tester->aux);

        if (message == NULL)
            printf(ANSI_COLOR_GREEN "PASS!" ANSI_COLOR_RESET "\n");
        else
        {
            printf(ANSI_COLOR_RED "FAIL!" ANSI_COLOR_RESET "\n");
            printf("    Message: %s\n", message);
        }
    }
}