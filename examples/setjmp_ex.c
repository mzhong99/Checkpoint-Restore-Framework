#include <stdio.h>
#include <setjmp.h>

jmp_buf env;

void sub()
{
    printf("subroutine is running\n");
    longjmp(env, 1);
}

int main()
{
    int rc;

    rc = setjmp(env);

    if (rc != 0)
        printf("return from longjmp\n");
    else
        sub();
}
