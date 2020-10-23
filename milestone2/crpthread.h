#ifndef __CRPTHREAD_H__
#define __CRPTHREAD_H__

#include <pthread.h>

typedef pthread_t crpthread_t;
typedef pthread_attr_t crpthread_attr_t;



int crpthread_create(crpthread_t *thread, const crpthread_attr_t *attr,
                     void *(*start_routine) (void *), void *arg);

int crpthread_join(crpthread_t thread, void **retval);


#endif
