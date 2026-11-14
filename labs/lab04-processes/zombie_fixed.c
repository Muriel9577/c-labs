#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        printf("CHILD: PID=%d, exiting now\n", getpid());
        return 0;
    }

    printf("PARENT: PID=%d, child=%d\n",
           getpid(),
           pid);

    wait(NULL);
    printf("PARENT: child collected, sleeping\n");

    sleep(60);
    return 0;
}
