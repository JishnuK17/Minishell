#include "header.h"

Slist *head = NULL;
extern int status;

/*builtin commands*/
        char *builtins[] = {"jobs", "fg", "bg", "echo", "printf", "read", "cd", "pwd", "pushd", "popd", "dirs", "let", "eval", "set", "unset", "export", "declare", "typeset", "readonly", "getopts", "source", "exit", "exec", "shopt", "caller", "true", "type", "hash", "bind", "help", NULL};

void extract_external_commands(char **external_commands)
{
        // open a file and read the commands till '\n'
        int fd = open("external_commands.txt", O_RDONLY);
        if (fd == -1)
        {
                perror("file open");
                return;
        }

        char ch;
        int len = 0, i = 0;
        // temporary storage for one command
        char buffer[100];
        int ret;
        while ((ret = read(fd, &ch, 1)) > 0)
        {
                if (ch != '\n')
                {
                        // take the length of the commad and get the command
                        buffer[len++] = ch;
                }
                else
                {
                        buffer[len] = '\0';
                        // allocate the col memory based on the length
                        external_commands[i] = (char *)malloc((len + 1) * sizeof(char));
                        // store the command into 2d array
                        strcpy(external_commands[i], buffer);

                        len = 0;
                        i++;
                }
        }
        if (len > 0)
        {
                buffer[len] = '\0';
                external_commands[i] = (char *)malloc((len + 1) * sizeof(char));
                strcpy(external_commands[i], buffer);
                i++;
        }
        external_commands[i] = NULL;
        close(fd);
}

char *get_command(char *input_str)
{
        static char command[10];
        // fetch only the command (input_str => ls -l --> command ls)
        int i = 0;
        while (input_str[i] != ' ' && input_str[i] != '\0')
        {
                command[i] = input_str[i];
                i++;
        }
        command[i] = '\0';

        return command;
}

int check_command_type(char *cmd, char **external_commands)
{
        // compare the cmd[] array with builtins[][]
        int i = 0;
        while (builtins[i])
        {
                if (strcmp(builtins[i], cmd) == 0)
                        return BUILTIN;
                i++;
        }

        // compare the cmd[] array with external_command[][]
        i = 0;
        while (external_commands[i])
        {
                if (strcmp(external_commands[i], cmd) == 0)
                        return EXTERNAL;
                i++;
        }

        // else
        return NO_COMMAND;
}

void execute_internal_commands(char *input_str)
{
        // commands to execute
        // 1. exit
        // 2. pwd
        // 3. cd
        // check the input string is exit or not
        if (strcmp(input_str, "exit") == 0)
                exit(0);
        // check the input string is pwd or not
        else if (strcmp(input_str, "pwd") == 0)
        {
                char buffer[100];
                getcwd(buffer, sizeof(buffer));
                printf("%s\n", buffer);
        }
        // check the input string has cd or not
        else if (strncmp(input_str, "cd", 2) == 0)
        {
                chdir(input_str + 3);
        }
        // check the input string is echo $$ or not
        else if (strcmp(input_str, "echo $$") == 0)
        {
                printf("%d\n", getpid());
        }
        // check the input string is echo $? or not
        else if (strcmp(input_str, "echo $?") == 0)
        {
                // check prev command execution
                if (WIFEXITED(status))
                {
                        printf("%d\n", WEXITSTATUS(status));
                }

        }
        // check the input string is echo $SHELL or not
        else if (strcmp(input_str, "echo $SHELL") == 0)
        {
                printf("%s\n", getenv("SHELL"));
        }
        // jobs, fg, bg
        // check the input string is jobs or not
        else if (strcmp(input_str, "jobs") == 0)
        {
                print_list(head);
        }
        // check the input string is fg or not
        else if (strcmp(input_str, "fg") == 0)
        {
                // check if there are any stopped process
                if (head != NULL)
                {
                        printf("%s\n", head->cmd);
                        kill(head->pid, SIGCONT);
                        waitpid(head->pid, &status, WUNTRACED);
                        delete_first(&head);
                }
        }
        // check the input string is bg or not
        else if (strcmp(input_str, "bg") == 0)
        {
                // check if there are any stopped process
                if (head != NULL)
                {
                        printf("%s\n", head->cmd);
                        kill(head->pid, SIGCONT);
                        delete_first(&head);
                        // register a signal to clear the resources when child changes state
                        signal(SIGCHLD, signal_handler);
                }
        }
}

void execute_external_commands(char *input_str)
{
        // convert 1D array of input string to 2D array (argv[][])
        int i = 0;
        char *argv[10]; // 2D

        char *token = strtok(input_str, " ");
        while (token != NULL)
        {
                argv[i++] = token;      // pointer to each 1D array
                token = strtok(NULL, " ");
        }
        argv[i] = NULL;
        int argc = i;

        // check if pipe is present or not
        int cmd_pos[10];
        cmd_pos[0] = 0;
        int cmd_count = 1;
        int pipe_flag = 0;

        for (i = 0; i < argc; i++)
        {
                if (strcmp(argv[i], "|") == 0)
                {
                        pipe_flag = 1;
                        // replace pipe with NULL
                        argv[i] = NULL;
                        // store next command index into cmd_pos array
                        cmd_pos[cmd_count++] = i + 1;
                        // keep track of no. of commands
                }
        }
        // not present
        if (pipe_flag == 0)
                execvp(argv[0], argv);
        // present
        else
        {
                int bck_stdin = dup(0);
                // n pipes
                for (i = 0; i < cmd_count; i++)
                {
                        // create pipe except for last command
                        int fd[2];
                        if (i < cmd_count - 1)
                        {
                                if (pipe(fd) == -1)
                                {
                                        perror("pipe");
                                        return;
                                }
                        }
                        // create child process
                        int pid = fork();
                        if (pid == 0)
                        {
                                // child process
                                // write to pipe if not last command
                                if (i < cmd_count - 1)
                                {
                                        close(fd[0]);
                                        dup2(fd[1], 1);
                                        close(fd[1]);
                                }
                                // execute command
                                execvp(argv[cmd_pos[i]], argv + cmd_pos[i]);
                        }
                        else if (pid > 0)
                        {
                                if (i < cmd_count - 1)
                                {
                                        // read from pipe
                                        close(fd[1]);
                                        dup2(fd[0], 0);
                                        close(fd[0]);
                                }
                        }
                }
                // wait for all children
                for(i = 0; i < cmd_count; i++)
                {
                        wait(NULL);
                }

                // restore stdin
                dup2(bck_stdin, 0);
                close(bck_stdin);
                exit(0);
        }
}