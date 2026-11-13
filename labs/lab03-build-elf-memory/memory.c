#include <stdio.h>
#include <stdlib.h>

int global_initialized = 10;
int global_uninitialized;
static int static_initialized = 20;
static int static_uninitialized;

int main(void)
{
    int local = 30;
    int *dynamic = malloc(sizeof(int));

    if (dynamic == NULL) {
        return 1;
    }
    *dynamic = 40;

    printf("Function main: %p\n", (void *)main);
    printf("Global initialized: %p\n", (void *)&global_initialized);
    printf("Global uninitialized: %p\n", (void *)&global_uninitialized);
    printf("Static initialized: %p\n", (void *)&static_initialized);
    printf("Static uninitialized: %p\n", (void *)&static_uninitialized);
    printf("Local variable: %p\n", (void *)&local);
    printf("Dynamic allocation: %p\n", (void *)dynamic);

    free(dynamic);
    return 0;
}
