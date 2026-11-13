#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("PID: %d\n", getpid());
    printf("Process is running...\n");
    sleep(60);
    return 0;
}
