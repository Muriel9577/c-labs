#include <stdio.h>
#include <stdlib.h>

int global_initialized = 10;
int global_uninitialized;
static int static_value = 20;

void function(void)
{
}

int main(void)
{
    int local = 30;
    int *dynamic = malloc(sizeof(*dynamic));

    if (dynamic == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    *dynamic = 40;

    printf("function: %p\n", (void *)function);
    printf("global initialized: %p\n", (void *)&global_initialized);
    printf("global uninitialized: %p\n", (void *)&global_uninitialized);
    printf("static: %p\n", (void *)&static_value);
    printf("heap: %p\n", (void *)dynamic);
    printf("stack: %p\n", (void *)&local);

    free(dynamic);
    return EXIT_SUCCESS;
}
