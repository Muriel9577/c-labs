#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

typedef struct {
    char *argv[MAX_ARGS];
    char *outfile;
} Command;

/* Split a line into tokens separated by spaces or tabs. */
static int tokenize(char *line, char **tokens)
{
    int count = 0;
    char *token = strtok(line, " \t");

    while (token != NULL) {
        if (count >= MAX_ARGS - 1) {
            fprintf(stderr, "myshell: too many arguments\n");
            return -1;
        }
        tokens[count++] = token;
        token = strtok(NULL, " \t");
    }

    tokens[count] = NULL;
    return count;
}

/* Build a Command from tokens; handles "> file". */
static int parse_command(char **tokens, int count, Command *cmd)
{
    int argc = 0;
    cmd->outfile = NULL;

    for (int i = 0; i < count; i++) {
        if (strcmp(tokens[i], ">") == 0) {
            if (i + 1 >= count || cmd->outfile != NULL) {
                fprintf(stderr, "myshell: syntax error near '>'\n");
                return -1;
            }
            cmd->outfile = tokens[++i];
        } else {
            cmd->argv[argc++] = tokens[i];
        }
    }

    cmd->argv[argc] = NULL;

    if (argc == 0) {
        fprintf(stderr, "myshell: empty command\n");
        return -1;
    }

    return 0;
}

/* Child side: restore default SIGINT (SIG_IGN would survive exec). */
static void child_restore_signals(void)
{
    struct sigaction action = {0};
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
}

/* Child side: redirect stdout to a file, exit on failure. */
static void child_redirect_output(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("open");
        _exit(EXIT_FAILURE);
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        close(fd);
        _exit(EXIT_FAILURE);
    }

    close(fd);
}

static void wait_for(pid_t pid)
{
    int status;

    while (waitpid(pid, &status, 0) == -1) {
        if (errno == EINTR) {
            continue;
        }
        perror("waitpid");
        return;
    }
}

static void run_simple(Command *cmd)
{
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        child_restore_signals();

        if (cmd->outfile != NULL) {
            child_redirect_output(cmd->outfile);
        }

        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");
        _exit(EXIT_FAILURE);
    }

    wait_for(pid);
}

static void run_pipeline(Command *left, Command *right)
{
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t child1 = fork();

    if (child1 == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (child1 == 0) {
        child_restore_signals();

        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }

        close(pipefd[0]);
        close(pipefd[1]);

        execvp(left->argv[0], left->argv);
        perror("execvp");
        _exit(EXIT_FAILURE);
    }

    pid_t child2 = fork();

    if (child2 == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        wait_for(child1);
        return;
    }

    if (child2 == 0) {
        child_restore_signals();

        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }

        close(pipefd[0]);
        close(pipefd[1]);

        if (right->outfile != NULL) {
            child_redirect_output(right->outfile);
        }

        execvp(right->argv[0], right->argv);
        perror("execvp");
        _exit(EXIT_FAILURE);
    }

    /* The parent does not use the pipe: close both ends. */
    close(pipefd[0]);
    close(pipefd[1]);

    wait_for(child1);
    wait_for(child2);
}

int main(void)
{
    /* The shell itself ignores Ctrl+C. */
    struct sigaction ignore = {0};
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);

    if (sigaction(SIGINT, &ignore, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    char line[MAX_LINE];

    for (;;) {
        printf("myshell> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            putchar('\n');
            break;
        }

        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        } else if (!feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }
            fprintf(stderr, "myshell: line too long\n");
            continue;
        }

        char *tokens[MAX_ARGS];
        int count = tokenize(line, tokens);

        if (count <= 0) {
            continue;
        }

        /* Built-ins run inside the shell itself (no fork). */
        if (strcmp(tokens[0], "exit") == 0) {
            break;
        }

        if (strcmp(tokens[0], "cd") == 0) {
            const char *target = tokens[1] != NULL ? tokens[1] : getenv("HOME");

            if (target == NULL) {
                fprintf(stderr, "cd: no directory given\n");
            } else if (chdir(target) == -1) {
                perror("cd");
            }
            continue;
        }

        /* Look for a single pipe. */
        int pipe_index = -1;
        int pipe_count = 0;

        for (int i = 0; i < count; i++) {
            if (strcmp(tokens[i], "|") == 0) {
                pipe_count++;
                pipe_index = i;
            }
        }

        if (pipe_count > 1) {
            fprintf(stderr, "myshell: only one pipe is supported\n");
            continue;
        }

        if (pipe_count == 0) {
            Command cmd;

            if (parse_command(tokens, count, &cmd) == 0) {
                run_simple(&cmd);
            }
        } else {
            Command left;
            Command right;

            if (pipe_index == 0 || pipe_index == count - 1) {
                fprintf(stderr, "myshell: syntax error near '|'\n");
                continue;
            }

            if (parse_command(tokens, pipe_index, &left) != 0 ||
                parse_command(tokens + pipe_index + 1,
                              count - pipe_index - 1, &right) != 0) {
                continue;
            }

            if (left.outfile != NULL) {
                fprintf(stderr,
                        "myshell: redirection on the left side of a pipe is not supported\n");
                continue;
            }

            run_pipeline(&left, &right);
        }
    }

    return EXIT_SUCCESS;
}
