#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *worker(void *arg)
{
    int id = *(int *)arg;
    int local = id * 10;

    printf("Thread %d: local=%d, address=%p\n", id, local, (void *)&local);

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

    return EXIT_SUCCESS;
}
