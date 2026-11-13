#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int *a = malloc(sizeof(int));
    int *b = malloc(sizeof(int));
    int *c = malloc(sizeof(int));

    if (a == NULL || b == NULL || c == NULL) {
        free(a);
        free(b);
        free(c);
        return 1;
    }

    printf("a: %p\n", (void *)a);
    printf("b: %p\n", (void *)b);
    printf("c: %p\n", (void *)c);

    free(a);
    free(b);
    free(c);
    return 0;
}
