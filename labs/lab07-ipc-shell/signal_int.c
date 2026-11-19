#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static void handle_sigint(int signal_number)
{
    (void)signal_number;

    const char message[] = "\nSIGINT received\n";
    write(STDOUT_FILENO, message, sizeof(message) - 1);
}

int main(void)
{
    struct sigaction action = {0};
    action.sa_handler = handle_sigint;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) == -1) {
        return EXIT_FAILURE;
    }

    while (1) {
        pause();
    }
}
