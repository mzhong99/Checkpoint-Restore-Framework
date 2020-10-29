#ifndef __VTHREADTABLE_H__
#define __VTHREADTABLE_H__

#include "memcheck.h"
#include "crthread.h"

#include <sys/types.h>
#include <stddef.h>

void vthreadtable_init();
void vthreadtable_cleanup();

void vthreadtable_insert(struct crthread *handle);
struct crthread *vthreadtable_find(pthread_t id);
struct crthread *vthreadtable_remove(pthread_t id);

#endif