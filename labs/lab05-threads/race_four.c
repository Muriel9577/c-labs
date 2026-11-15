#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define THREADS 4
#define ITERATIONS 500000

long counter = 0;

void *increment(void *arg)
{
    (void)arg;

    for (long i = 0; i < ITERATIONS; i++) {
        counter++;
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[THREADS];

    for (int i = 0; i < THREADS; i++) {
        if (pthread_create(&threads[i], NULL, increment, NULL) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Expected: %ld\n", (long)THREADS * ITERATIONS);
    printf("Actual: %ld\n", counter);

    return EXIT_SUCCESS;
}
