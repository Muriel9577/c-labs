#include <stdio.h>
#include <unistd.h>

int main(void)
{
    fork();
    fork();
    fork();
    printf("PID=%d PPID=%d\n", getpid(), getppid());
    sleep(1);
    return 0;
}
