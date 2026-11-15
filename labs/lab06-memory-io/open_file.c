#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    int fd = open("example.txt",
                  O_WRONLY | O_CREAT | O_TRUNC,
                  0644);

    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    printf("Opened file descriptor: %d\n", fd);
    fflush(stdout);

    sleep(30);

    close(fd);
    return EXIT_SUCCESS;
}
