#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        int fd = open("ls-output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd == -1) {
            perror("open");
            _exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(fd);
            _exit(EXIT_FAILURE);
        }

        close(fd);

        char *args[] = {"ls", "-l", NULL};
        execvp(args[0], args);

        perror("execvp");
        _exit(EXIT_FAILURE);
    }

    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
