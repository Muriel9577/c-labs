#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    printf("Before exec: PID=%d\n", getpid());
    fflush(stdout);

    execl("./new_program", "new_program", NULL);

    perror("execl");
    return EXIT_FAILURE;
}
