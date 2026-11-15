#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *worker(void *arg)
{
    (void)arg;
    printf("Hello from worker thread!\n");
    return NULL;
}

int main(void)
{
    pthread_t thread;

    printf("Main: creating thread\n");

    int result = pthread_create(&thread, NULL, worker, NULL);

    if (result != 0) {
        fprintf(stderr, "pthread_create failed: %d\n", result);
        return EXIT_FAILURE;
    }

    pthread_join(thread, NULL);
    printf("Main: worker finished\n");

    return EXIT_SUCCESS;
}
