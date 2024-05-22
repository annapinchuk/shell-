#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>

int main() {
    char command[1024];
    char *token;
    char *outfile;
    int i, fd, amper, redirect, redirect_err, append, retid, status;
    char *argv[10];

    while (1) {
        printf("hello: ");
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';

        /* parse command line */
        i = 0;
        token = strtok(command, " ");
        while (token != NULL) {
            argv[i] = token;
            token = strtok(NULL, " ");
            i++;
        }
        argv[i] = NULL;

        /* Is command empty */
        if (argv[0] == NULL)
            continue;

        /* Initialize flags */
        amper = 0;
        redirect = 0;
        redirect_err = 0;
        append = 0;
        outfile = NULL;

        /* Parse command line for special characters */
        for (int j = 0; argv[j] != NULL; j++) {
            if (!strcmp(argv[j], "&")) {
                amper = 1;
                argv[j] = NULL;
            } else if (!strcmp(argv[j], ">")) {
                redirect = 1;
                append = 0;
                argv[j] = NULL;
                outfile = argv[j + 1];
            } else if (!strcmp(argv[j], ">>")) {
                redirect = 1;
                append = 1;
                argv[j] = NULL;
                outfile = argv[j + 1];
            } else if (!strcmp(argv[j], "2>")) {
                redirect_err = 1;
                argv[j] = NULL;
                outfile = argv[j + 1];
            }
        }

        /* for commands not part of the shell command language */
        if (fork() == 0) {
            /* redirection of stdout */
            if (redirect) {
                if (append) {
                    fd = open(outfile, O_WRONLY | O_APPEND | O_CREAT, 0660);
                } else {
                    fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            /* redirection of stderr */
            if (redirect_err) {
                fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }

            execvp(argv[0], argv);
            perror("execvp failed");  // Print error message if execvp fails
            exit(1);  // Exit with error code
        }

        /* parent continues here */
        if (amper == 0)
            retid = wait(&status);
    }
}
