#include "contextswitch.h"
#include <stdio.h>

void display_context(struct crcontext *volatile context)
{
    uint64_t returnaddr;

    printf("Context ptr: %p\n", context);
    returnaddr = *(uint64_t *)(context->rbp + 8);

    printf("Context:\n");
    printf("----------------------------------------\n");

    printf("    rsp: 0x%lx, *rsp: 0x%lx\n", 
           context->rsp, *(uint64_t *)context->rsp);

    printf("    rbp: 0x%lx, *rbp: 0x%lx\n", 
           context->rbp, *(uint64_t *)context->rbp);

    printf("    return address [*(rbp + 8)]: 0x%lx\n", returnaddr);
}