#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    printf("Read end: %d\n", pipefd[0]);
    printf("Write end: %d\n", pipefd[1]);

    const char message[] = "Hello through pipe!\n";

    if (write(pipefd[1], message, strlen(message)) == -1) {
        perror("write");
        return EXIT_FAILURE;
    }

    char buffer[128];
    ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);

    if (bytes_read == -1) {
        perror("read");
        return EXIT_FAILURE;
    }

    buffer[bytes_read] = '\0';
    printf("Received: %s", buffer);

    close(pipefd[0]);
    close(pipefd[1]);

    return EXIT_SUCCESS;
}
