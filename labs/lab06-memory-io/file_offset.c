#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    char buffer[6];

    int fd = open("input.txt", O_RDONLY);

    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    ssize_t n1 = read(fd, buffer, 5);

    if (n1 > 0) {
        buffer[n1] = '\0';
        printf("First: [%s]\n", buffer);
    }

    ssize_t n2 = read(fd, buffer, 5);

    if (n2 > 0) {
        buffer[n2] = '\0';
        printf("Second: [%s]\n", buffer);
    }

    close(fd);
    return EXIT_SUCCESS;
}
