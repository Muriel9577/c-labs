#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("New program PID=%d\n", getpid());
    return 0;
}
