#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("STDIN = %d\n", STDIN_FILENO);
    printf("STDOUT = %d\n", STDOUT_FILENO);
    printf("STDERR = %d\n", STDERR_FILENO);
    return 0;
}
