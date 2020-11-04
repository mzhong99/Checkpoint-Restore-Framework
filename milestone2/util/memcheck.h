#ifndef __MEMCHECK_H__
#define __MEMCHECK_H__

#include <stddef.h>
#include <sys/mman.h>

void *mcmalloc(size_t size);
void *mccalloc(size_t nmemb, size_t size);
void *mcrealloc(void *ptr, size_t size);
void mcfree(void *ptr);

void *mcmmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int mcmunmap(void *addr, size_t len);

void mcreport();

#endif
