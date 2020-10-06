#include <unistd.h>

#include <stdio.h>
#include <stdarg.h>

int crprintf(const char * __restrict fmt, ...)
{
    static char buffer[256];
    va_list list;
    int i, nwrite;

    va_start(list, fmt);
    nwrite = vsnprintf(buffer, sizeof(buffer), fmt, list);
    va_end(list);

    return write(STDOUT_FILENO, buffer, nwrite);
}

int main(int argc, char **argv)
{
    crprintf("[TEST BEGIN] Beginning tests for <crmalloc>\n");

    crprintf("[TEST END] Finishing tests for <crmalloc>\n");
}
