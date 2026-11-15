#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    char buffer[16];

    int fd = open("input.txt", O_RDONLY);

    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        if (write(STDOUT_FILENO, buffer, (size_t)bytes_read) == -1) {
            perror("write");
            close(fd);
            return EXIT_FAILURE;
        }
    }

    if (bytes_read == -1) {
        perror("read");
    }

    close(fd);

    return bytes_read == -1 ? EXIT_FAILURE : EXIT_SUCCESS;
}
