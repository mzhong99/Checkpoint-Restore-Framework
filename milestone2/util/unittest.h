#ifndef __UNITTEST_H__
#define __UNITTEST_H__

#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_YELLOW   "\x1b[33m"
#define ANSI_COLOR_BLUE     "\x1b[34m"
#define ANSI_COLOR_MAGENTA  "\x1b[35m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_RESET    "\x1b[0m"

#include <stddef.h>

#define CRASH_FOREVER   (0xFFFFFFFF)
#define CRASH_INTERVAL  (1000000)

struct checkpoint_tester
{
    void *aux;
    void (*setup)(void **);
    void (*test)(void *);
    const char * (*check_answer)(void *);

    size_t num_allowed_crashes;
    unsigned int crash_length_us;
};

void run_test(const char *(*test)(), const char *name, const char *description);
void run_concurrent_test(struct checkpoint_tester *tester, 
                         const char *name, const char *description);

#endif
