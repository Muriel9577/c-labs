#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* Child reads */
        close(pipefd[1]);

        char buffer[128];
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);

        if (n == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        buffer[n] = '\0';
        printf("Child received: %s", buffer);

        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    }

    /* Parent writes */
    close(pipefd[0]);

    const char message[] = "Message from parent\n";

    if (write(pipefd[1], message, strlen(message)) == -1) {
        perror("write");
    }

    close(pipefd[1]);
    waitpid(pid, NULL, 0);

    return EXIT_SUCCESS;
}
