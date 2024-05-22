#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <sys/types.h>

char *prompt_str = "hello";
#define  SIZE 10000

void pipe_func(int num_pipes,char * pipes [100][1000],int fildes[1000][2],int i,int last_pipe_size){
    if(i==0){
        close(STDOUT_FILENO);
        dup(fildes[0][1]);
        close(fildes[0][1]);
        close(fildes[0][0]);

        execvp(pipes[0][0], pipes[0]);
        perror("Error executing command");
        exit(1);
    }
    pipe(fildes[i-1]);
    int pid;
    if ((pid = fork()) == -1)
    {
        perror("Error forking");
    }
    else if (pid == 0)
    {
        pipe_func(num_pipes,pipes,fildes,i-1,last_pipe_size);
    }
    else{
        /* Child process */

        if (i == num_pipes-1)
        {

            if ((last_pipe_size > 1) && !strcmp(pipes[i][last_pipe_size - 2], ">"))
            {
                pipes[i][last_pipe_size - 2] = NULL;
                int fd = creat(pipes[i][last_pipe_size - 1], 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
            }
            close(STDIN_FILENO);
            dup(fildes[i-1][0]);
            close(fildes[i-1][1]);
            close(fildes[i-1][0]);

        }
        else
        {
            close(STDIN_FILENO);
            dup(fildes[i-1][0]);
            close(STDOUT_FILENO);
            dup(fildes[i][1]);


            close(fildes[i-1][1]);
            close(fildes[i-1][0]);
            /* Middle commands */
            close(fildes[i - 1][1]);
            close(fildes[i][0]);

        }
        /* Execute command */
        execvp(pipes[i][0], pipes[i]);
        perror("Error executing command");
        exit(1);
    }
}


void cHandler(int dummy)
{
    printf("You typed Control-C!\n");
    printf("%s: ", prompt_str);
    fflush(stdout);
}

int main()
{
    signal(SIGINT, cHandler);

    char command[SIZE],command2[SIZE],then_command[SIZE],else_command[SIZE];
    int pipes_fd[1000][2] = {0};
    char *token;
    char *outfile;
    char *errfile;
    int i = 0, fd = 0, fd_err = 0, amper = 0, redirect = 0, retid = 0, status = 0, piping = 0, argc1 = 0,if_cond =-10,then_cond=-10,index=0,else_cond=-10,num_pipes=0,last_pipe_size;
    int fildes[2] = {0};
    char *argv[100][1000] = {0};
    char* words[1000] = {0};
    char keys[1000][1000] = {0}, values[1000][1000] = {0};
    char memory[20][1024] = {0};
    int location = 0, map_index = 0;
    char c = '\0';
    while (1)
    {
        index++;
        if(if_cond!=index-1 && then_cond!=index-1 && then_cond!=index-2 && else_cond!=index-1 && else_cond!=index-2){
            printf("%s: ", prompt_str);
        }
        fgets(command2, 1024, stdin);
        command2[strlen(command2) - 1] = '\0';

        // everytime he presses up or down i present the previous/next command. once he presses enter i execute that command and exit the arrow mode
        int counter = location;
        while (command2[0] == '\033')
        {
                    // system ("/bin/stty raw");

        // system ("/bin/stty cooked");
            switch (command2[2])
            { // the real value
            case 'A':
                // code for arrow up
                counter = (counter+19) % 20;
                //maybe add if counter= location that we went through all 20 commands

                if (memory[counter][0] == '\0')
                { // reached the downmost command - stay on same previous command
                    counter = (counter+1) % 20;
                }
                //present to the user the command which he navigated to
                printf("%s\n", memory[counter]);
                break;
            case 'B':
                // code for arrow down
                counter = (counter + 1) % 20;
                //maybe add if counter= location that we went through all 20 commands

                if (memory[counter][0] == '\0')
                { // reached the downmost command - stay on same previous command
                    counter = (counter+19) % 20;
                }
                //present to the user the command which he navigated to
                printf("%s\n", memory[counter]);
                break;
            }
            fgets(command2, 1024, stdin);
            if (command2[0] == '\n')//allow the user to press enter and choose the command we presented to him
            {
                for (int i = 0; i < 1024; i++)
                {
                    command2[i] = memory[counter][i];
                }
                break;
            }
            else //user pressed another arrow -  continue to next iteration
            {
                command2[strlen(command2) - 1] = '\0';//(need this in else otherwise seg fault)
            }
        }
        if(command2[0] == '\0'){
            printf("empty command\n");
            continue;
        }

        i = 0;
        words[i++] = strtok(command2, " ");
        while ((words[i] = strtok(NULL, " ")) != NULL)
            i++;

        if (words[0][0] == '$' && (i == 3) && (!strcmp(words[1], "=")))
        {
            memcpy(keys[map_index], words[0], strlen(words[0]));
            memcpy(values[map_index++] , words[2], strlen(words[2]));
            continue;
        }

        for (int j = 0; j < i; j++){
            if (words[j][0]=='$' && words[j][1]!='?'){
                for (int k = map_index; k >=0 ; k--)
                {
                    if(! strcmp(words[j],keys[k])){
                        words[j]=values[k];
                        break;
                    }
                }
            }
        }

        int c=0;
        for (int j = 0; j < i; j++){
            for (int k = 0; k < strlen(words[j]); k++)
            {
                command[c++]=words[j][k];
            }
            if(j!=i-1) {
                command[c++] = ' ';
            }
        }
        command[c] = '\0';

        piping = 0;
        if ((command[0] == '!') && (command[1] == '!') && (command[2] == '\0'))
        {
            for (int i = 0; i < 1024; i++)
            {
                command[i] = memory[location - 1][i];
            }
        }
        else
        {
            for (int i = 0; i < 1024; i++)
            {
                memory[location][i] = command[i];
            }
            location = (location + 1) % 20;
        }

        if(then_cond==index-1){
            strcpy(then_command, command);
            continue;
        }
        if(else_cond==index-1){
            strcpy(else_command, command);
            continue;
        }
        if(!strcmp(command,"fi")){
            if(!status){
                strcpy(command, then_command);
            }
            else{
                strcpy(command, else_command);
            }
        }
        
        i = 0;
        token = strtok (command,"|");
        char* args_pipe[100];
        while (token != NULL)
        {
            args_pipe[i] = token;
            token = strtok (NULL, "|");
            i++;
        }
        if(i>1){
            piping=1;
        }
        args_pipe[i] = NULL;
        num_pipes=i;

        for (int j = 0; j < i; j++){
            int k=0;
            token = strtok (args_pipe[j]," ");
            while (token != NULL)
            {
                argv[j][k] = token;
                token = strtok (NULL, " ");
                k++;
            }
            if(j==0){
                argc1=k;
            }
            if(j==i-1){
                last_pipe_size=k;
            }
            argv[j][k] = NULL;
        }

        /* Is command empty */
        if (argv[0][0] == NULL)
            continue;

        if (!strcmp(argv[0][0], "if")){
            if_cond=index;
            for (int i = 0; i < argc1; i++)
            {
                argv[0][i]=argv[0][i+1];
            }
            memset(else_command, '\0', sizeof(else_command));
            memset(then_command, '\0', sizeof(then_command));
            argc1--;
        }

        if (!strcmp(argv[0][0], "then") && (argc1 == 1) && if_cond==index-1){
            then_cond=index;
            continue;
        }

        if (!strcmp(argv[0][0], "else") && (argc1 == 1) && then_cond==index-2){
            else_cond=index;
            continue;
        }


        if (!strcmp(argv[0][0], "read") && (argc1 == 2))
        {
            char str[1024];
            fgets(str, 1024, stdin);
            str[strlen(str) - 1] = '\0';
            keys[map_index][0]='$';
            memcpy(keys[map_index]+1, argv[0][1], strlen(argv[0][1]));
            memcpy(values[map_index++] , str, strlen(str));
            continue;
        }


        if (!strcmp(argv[0][0], "quit"))
        { // allow user to exit
            printf("exiting the program\n");
            exit(0);
        }

        if (!strcmp(argv[0][argc1 - 1], "!!"))
        {
            amper = 1;
            argv[0][argc1 - 1] = NULL;
        }

        /* Does command line end with & */
        if (!strcmp(argv[0][argc1 - 1], "&"))
        {
            amper = 1;
            argv[0][argc1 - 1] = NULL;
        }
        else
            amper = 0;
        /* Changing directory*/
        if (!strcmp(argv[0][0], "cd"))
        {
            chdir(argv[0][1]);
            continue;
        }


        if ((!strcmp(argv[0][0], "prompt")) && (argc1 == 3) && (!strcmp(argv[0][1], "=")))
        {
            prompt_str = argv[0][2];
            continue;
        }
        if (!strcmp(argv[0][0], "echo") && (argc1 == 2) && !strcmp(argv[0][1], "$?"))
        {
            printf("%d\n", status);
            continue;
        }

        // supporting a chain of redirection first error then output
        if ((argc1 > 3) && (!strcmp(argv[0][argc1 - 4], ">")) && (!strcmp(argv[0][argc1 - 2], "2>")))
        {
            redirect = 3;
            argv[0][argc1 - 4] = NULL;
            outfile = argv[0][argc1 - 3];
            errfile = argv[0][argc1 - 1];
        }
            // supporting a chain of redirection first output then error
        else if ((argc1 > 3) && (!strcmp(argv[0][argc1 - 4], "2>")) && (!strcmp(argv[0][argc1 - 2], ">")))
        {
            redirect = 3;
            argv[0][argc1 - 4] = NULL;
            outfile = argv[0][argc1 - 1];
            errfile = argv[0][argc1 - 3];
        }
        else if ((argc1 > 1) && !strcmp(argv[0][argc1 - 2], ">"))
        {
            redirect = 1;
            argv[0][argc1 - 2] = NULL;
            outfile = argv[0][argc1 - 1];
        }
        else if ((argc1 > 1) && !strcmp(argv[0][argc1 - 2], "2>"))
        {
            redirect = 2;
            argv[0][argc1 - 2] = NULL;
            errfile = argv[0][argc1 - 1];
        }
        else if ((argc1 > 1) && !strcmp(argv[0][argc1 - 2], ">>"))
        {
            redirect = 4;
            argv[0][argc1 - 2] = NULL;
            outfile = argv[0][argc1 - 1];
        }
        else
            redirect = 0;

        /* for commands not part of the shell command language */

        if (fork() == 0)
        {
            /* redirection of IO ? */
            if (redirect == 1 && if_cond!=index)
            {
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
                /* stdout is now redirected */
            }
            /* redirection of ERR */
            if (redirect == 2 && if_cond!=index)
            {
                fd = creat(errfile, 0660);
                close(STDERR_FILENO);
                dup(fd);
                close(fd);
                /* stderr is now redirected */
            }
            if (redirect == 3 && if_cond!=index)
            {
                fd_err = creat(errfile, 0660);
                close(STDERR_FILENO);
                dup(fd_err);
                close(fd_err);
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
            }
            if (redirect == 4 && if_cond!=index)
            {
                fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
                /* stdout is now redirected and appended to outfile*/
            }
            if (piping){
                pipe_func(num_pipes,argv,pipes_fd,num_pipes-1,last_pipe_size);

            }

            else
            {
                if(if_cond==index){
                    freopen("/dev/null", "w", stdout);
                }
                execvp(argv[0][0], argv[0]);
                exit(1);
            }
        }
        /* parent continues here */
        if (amper == 0)
            retid = wait(&status);
    }
}