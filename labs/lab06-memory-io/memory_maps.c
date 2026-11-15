#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int global_value = 10;

int main(void)
{
    int local = 20;
    int *dynamic = malloc(sizeof(*dynamic));

    if (dynamic == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    *dynamic = 30;

    printf("PID: %d\n", getpid());
    printf("main: %p\n", (void *)main);
    printf("global: %p\n", (void *)&global_value);
    printf("heap: %p\n", (void *)dynamic);
    printf("stack: %p\n", (void *)&local);
    fflush(stdout);

    sleep(60);

    free(dynamic);
    return EXIT_SUCCESS;
}
