#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("Output using printf\n");

    const char message[] = "Output using write\n";
    write(STDOUT_FILENO, message, sizeof(message) - 1);

    return 0;
}
