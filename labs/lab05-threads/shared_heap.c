#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int value;
} SharedData;

void *worker(void *arg)
{
    SharedData *data = arg;

    data->value += 10;

    printf("Worker: value=%d address=%p\n", data->value, (void *)data);

    return NULL;
}

int main(void)
{
    SharedData *data = malloc(sizeof(*data));

    if (data == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    data->value = 100;

    pthread_t thread;
    pthread_create(&thread, NULL, worker, data);
    pthread_join(thread, NULL);

    printf("Main: value=%d address=%p\n", data->value, (void *)data);

    free(data);
    return EXIT_SUCCESS;
}
