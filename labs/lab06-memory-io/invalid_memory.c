#include <stdio.h>

int main(void)
{
    int *pointer = NULL;

    printf("About to access invalid memory\n");
    fflush(stdout);

    *pointer = 42;

    printf("You should not normally see this\n");
    return 0;
}
