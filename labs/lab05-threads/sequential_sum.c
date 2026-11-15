#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    long n = 1000000;

    if (argc == 2) {
        n = strtol(argv[1], NULL, 10);
    }

    long sum = 0;

    for (long i = 1; i <= n; i++) {
        sum += i;
    }

    printf("Sequential result: %ld\n", sum);
    return EXIT_SUCCESS;
}
