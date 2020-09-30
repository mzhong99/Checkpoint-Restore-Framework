#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int fd, rc;
    char *addr;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(argv[1], O_CREAT | O_RDWR);
    printf("fd: %d\n", fd);
    rc = posix_fallocate(fd, 0, 1024);
    if (rc != 0)
        printf("FAILED ALLOCATE, %d\n", rc);

    addr = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (addr == MAP_FAILED)
        printf("mmap() failed.\n");
    else
    {
        sprintf(addr, "Hello, world!");
        printf("%s\n", addr);
    }

    munmap(addr, 1024);
    close(fd);
    printf("DONE\n");
}
