#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

static void handler(int signal_number)
{
    (void)signal_number;
}

int main(void)
{
    struct sigaction action = {0};
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGKILL, &action, NULL) == -1) {
        printf("sigaction(SIGKILL) failed: %s\n", strerror(errno));
    } else {
        printf("sigaction(SIGKILL) succeeded\n");
    }

    if (sigaction(SIGTERM, &action, NULL) == -1) {
        printf("sigaction(SIGTERM) failed: %s\n", strerror(errno));
    } else {
        printf("sigaction(SIGTERM) succeeded\n");
    }

    return 0;
}
