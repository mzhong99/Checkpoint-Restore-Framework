#include <stdio.h>
#include <stdint.h>

struct crcontext
{
    volatile uint64_t addr;
    volatile uint64_t rsp;
    volatile uint64_t rbp;
} __attribute__((packed));

extern int save_context(struct crcontext *context);
extern void load_context(struct crcontext *context);

extern int save_jmp_addr(void **theaddr);

void test_save_jmp_addr()
{
    int retval;
    void *addr;

    printf("Starting...\n");

    retval = save_jmp_addr(&addr);
    printf("Where was I just at? %p, %d\n", addr, retval);

    retval = save_jmp_addr(&addr);
    printf("How about now? %p, %d\n", addr, retval);

    printf("Done.\n");
}

void test_save_context()
{
    struct crcontext context;
    int retval;

    printf("Starting...\n");

    retval = save_context(&context);
    printf("Where was I just at? %x, %d\n", context.addr, retval);

    retval = save_context(&context);
    printf("How about now? %x, %d\n", context.addr, retval);

    printf("Done.\n");
}

void test_load_context()
{
    struct crcontext context;

    printf("Starting...\n");
    if (save_context(&context) == 0)
    {
        printf("Context saved. Next, we reload context...\n");
        load_context(&context);
    }
    else
    {
        printf("Load context completed.\n");
    }
}

int main()
{
    test_load_context();
}
