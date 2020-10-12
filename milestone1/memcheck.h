#ifndef __MEMCHECK_H__
#define __MEMCHECK_H__

#include <stddef.h>
#include <sys/mman.h>

void *mc_malloc(size_t size);
void *mc_calloc(size_t nmemb, size_t size);
void *mc_realloc(void *ptr, size_t size);
void mc_free(void *ptr);

void *mc_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int mc_munmap(void *addr, size_t len);

void mc_report();

#endif
