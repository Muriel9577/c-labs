#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define THREADS 4

typedef struct {
    long start;
    long end;
    long result;
} Task;

void *calculate_sum(void *arg)
{
    Task *task = arg;
    long sum = 0;

    for (long i = task->start; i <= task->end; i++) {
        sum += i;
    }

    task->result = sum;
    return NULL;
}

int main(int argc, char *argv[])
{
    long n = 1000000;

    if (argc == 2) {
        n = strtol(argv[1], NULL, 10);
        if (n < THREADS) {
            fprintf(stderr, "N must be at least %d\n", THREADS);
            return EXIT_FAILURE;
        }
    }

    pthread_t threads[THREADS];
    Task tasks[THREADS];
    long chunk = n / THREADS;

    for (int i = 0; i < THREADS; i++) {
        tasks[i].start = (long)i * chunk + 1;
        tasks[i].end = (i == THREADS - 1) ? n : (long)(i + 1) * chunk;
        tasks[i].result = 0;

        if (pthread_create(&threads[i], NULL, calculate_sum, &tasks[i]) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            return EXIT_FAILURE;
        }
    }

    long total = 0;

    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
        printf("Thread %d: %ld ... %ld = %ld\n",
               i + 1, tasks[i].start, tasks[i].end, tasks[i].result);
        total += tasks[i].result;
    }

    long expected = n * (n + 1) / 2;

    printf("Parallel result: %ld\n", total);
    printf("Expected result: %ld\n", expected);
    printf("Correct: %s\n", total == expected ? "YES" : "NO");

    return EXIT_SUCCESS;
}
