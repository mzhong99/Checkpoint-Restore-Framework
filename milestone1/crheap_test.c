#include "crheap.h"

int main()
{
    crheap_init(NULL);
    crprintf("[TEST START] Beginning tests for module [crheap]\n");

    crheap_shutdown();
    crprintf("[TEST FINISH] Ending tests for module [crheap]\n");
}
