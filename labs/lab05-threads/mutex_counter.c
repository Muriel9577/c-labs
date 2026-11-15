#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 1000000

long counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *increment(void *arg)
{
    (void)arg;

    for (long i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&mutex);
        counter++;
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(void)
{
    pthread_t thread1;
    pthread_t thread2;

    pthread_create(&thread1, NULL, increment, NULL);
    pthread_create(&thread2, NULL, increment, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Expected: %ld\n", 2L * ITERATIONS);
    printf("Actual: %ld\n", counter);

    pthread_mutex_destroy(&mutex);

    return EXIT_SUCCESS;
}
