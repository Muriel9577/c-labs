#include <stdio.h>
#include <stdlib.h>
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
    printf("PARENT: sleeping without wait()\n");

    sleep(60);
    return 0;
}
