#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int value = 0;

void *worker(void *arg)
{
    int id = *(int *)arg;

    value = id * 100;

    printf("Thread %d: value=%d, address=%p\n", id, value, (void *)&value);

    return NULL;
}

int main(void)
{
    pthread_t threads[2];
    int ids[2] = {1, 2};

    pthread_create(&threads[0], NULL, worker, &ids[0]);
    pthread_create(&threads[1], NULL, worker, &ids[1]);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    printf("Main: value=%d, address=%p\n", value, (void *)&value);

    return EXIT_SUCCESS;
}
