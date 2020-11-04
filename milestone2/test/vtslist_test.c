#include "vtslist_test.h"
#include "vtslist.h"
#include "macros.h"
#include "memcheck.h"

#include <pthread.h>

#define VTSLIST_TEST_RANGE  100000

struct vtslist_tester
{
    size_t value;
    struct vtslist_elem vtselem;
};

static void *consumer(void *vtslist_vp)
{
    struct vtslist *vtslist;
    struct vtslist_elem *vtselem;
    struct vtslist_tester *tester;

    size_t i;

    vtslist = vtslist_vp;

    for (i = 0; i < VTSLIST_TEST_RANGE; i++)
    {
        vtselem = vtslist_pop_front(vtslist);
        tester = container_of(vtselem, struct vtslist_tester, vtselem);
        if (tester->value != i)
            return "Contents did not match (queue).";

        mcfree(tester);
    }

    return NULL;
}

const char *test_vtslist_message_passing()
{
    struct vtslist vtslist;
    struct vtslist_tester *tester;

    size_t i;
    pthread_t ptid;
    const char *result;

    vtslist_init(&vtslist);
    pthread_create(&ptid, NULL, consumer, &vtslist);

    for (i = 0; i < VTSLIST_TEST_RANGE; i++)
    {
        tester = mcmalloc(sizeof(*tester));
        tester->value = i;
        vtslist_push_back(&vtslist, &tester->vtselem);
    }

    pthread_join(ptid, (void **)&result);
    return result;
}