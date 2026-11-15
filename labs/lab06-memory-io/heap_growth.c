#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    const size_t size = 100 * 1024 * 1024;

    printf("PID: %d\n", getpid());

    char *memory = malloc(size);

    if (memory == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    printf("Allocated %zu bytes\n", size);
    printf("Address: %p\n", (void *)memory);
    printf("Press Enter to touch memory...\n");
    fflush(stdout);
    getchar();

    for (size_t i = 0; i < size; i += 4096) {
        memory[i] = 1;
    }

    printf("Memory touched. Press Enter to exit...\n");
    fflush(stdout);
    getchar();

    free(memory);
    return EXIT_SUCCESS;
}
