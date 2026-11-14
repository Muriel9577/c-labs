#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    printf("Before fork: PID=%d\n", getpid());
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        printf("CHILD: PID=%d, PPID=%d, fork returned=%d\n",
               getpid(),
               getppid(),
               pid);
    } else {
        printf("PARENT: PID=%d, child PID=%d, fork returned=%d\n",
               getpid(),
               pid,
               pid);
    }

    return EXIT_SUCCESS;
}
