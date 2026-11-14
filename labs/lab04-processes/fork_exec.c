#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    printf("PARENT: PID=%d\n", getpid());
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        printf("CHILD before exec: PID=%d\n", getpid());
        fflush(stdout);

        execlp("echo",
               "echo",
               "Program started by child",
               NULL);

        perror("execlp");
        _exit(EXIT_FAILURE);
    }

    printf("PARENT: created child PID=%d\n", pid);
    wait(NULL);
    printf("PARENT: child completed\n");

    return EXIT_SUCCESS;
}
