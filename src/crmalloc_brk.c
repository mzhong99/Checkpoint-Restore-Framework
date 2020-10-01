#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

struct crheap
{
    void *heap_start;
};



void *heap_start = NULL;

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
    heap_start = sbrk(0);
    crprintf("POINTER: %p\n", heap_start);
}
