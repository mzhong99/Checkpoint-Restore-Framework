#ifndef __CONTEXTSWITCH_H__
#define __CONTEXTSWITCH_H__

#include <stdint.h>

struct crcontext
{
    volatile uint64_t addr;
    volatile uint64_t rsp;
    volatile uint64_t rbp;
};

extern int save_context(volatile struct crcontext *volatile context);
extern int load_context(volatile struct crcontext *volatile context);

void display_context(struct crcontext *volatile context);

#endif