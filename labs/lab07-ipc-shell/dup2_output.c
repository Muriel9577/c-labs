#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    int fd = open("redirected.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);

    printf("This goes to the file\n");
    fflush(stdout);

    return EXIT_SUCCESS;
}
