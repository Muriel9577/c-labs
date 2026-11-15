#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    const char message[] = "Hello from write()\n";

    int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    ssize_t written = write(fd, message, strlen(message));

    if (written == -1) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Requested: %zu bytes\n", strlen(message));
    printf("Written: %zd bytes\n", written);

    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
