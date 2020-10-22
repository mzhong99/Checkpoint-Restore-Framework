#include "crpthread.h"
#include "crheap.h"
#include "nvstore.h"

#include "list.h"

#include <pthread.h>
#include <setjmp.h>

#define DEFAULT_STACKSIZE   8192

struct crthread
{
    struct list_elem elem;

    void *stack;
    size_t stacksize;

    jmp_buf checkpoint;

    void *(*taskfunc) (void *);
    void *arg;
    void *retval;

    pthread_t ptid;
};

struct crthread *crthread_new()
{
    struct crthread *thread;
    struct nvmetadata *metadata;

    metadata = nvmetadata_instance();

    thread = crmalloc(sizeof(*thread));
    thread->stacksize = DEFAULT_STACKSIZE;
    thread->ptid = -1;
    thread->taskfunc = NULL;

    pthread_mutex_lock(&metadata->threadlock);
    list_push_back(&metadata->threadlist, &thread->elem);
    pthread_mutex_unlock(&metadata->threadlock);

    return thread;
}
