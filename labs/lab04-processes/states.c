#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("PID=%d\n", getpid());
    fflush(stdout);
    sleep(60);
    return 0;
}
