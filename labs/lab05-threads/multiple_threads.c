#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *worker(void *arg)
{
    int id = *(int *)arg;
    printf("Thread %d is running\n", id);
    return NULL;
}

int main(void)
{
    enum { THREAD_COUNT = 4 };

    pthread_t threads[THREAD_COUNT];
    int ids[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; i++) {
        ids[i] = i + 1;

        int result = pthread_create(&threads[i], NULL, worker, &ids[i]);

        if (result != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", result);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All threads finished\n");
    return EXIT_SUCCESS;
}
