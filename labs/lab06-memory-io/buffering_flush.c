#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("A");
    printf("B");
    printf("C");

    fflush(stdout);

    write(STDOUT_FILENO, "D", 1);

    return 0;
}
