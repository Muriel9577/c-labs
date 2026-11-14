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
        printf("CHILD: working...\n");
        sleep(2);
        printf("CHILD: finished\n");
        return 42;
    }

    printf("PARENT: waiting for child %d\n", pid);

    int status;
    if (wait(&status) < 0) {
        perror("wait");
        return EXIT_FAILURE;
    }

    printf("PARENT: child finished\n");

    if (WIFEXITED(status)) {
        printf("PARENT: child exit status = %d\n",
               WEXITSTATUS(status));
    }

    return EXIT_SUCCESS;
}
