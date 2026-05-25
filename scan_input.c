#include "header.h"

extern Slist *head;
extern char prompt_str[];
extern char input_str[25];

char *external_commands[152];

int pid = 0;
int status = 0;

void signal_handler(int signum)
{
        if (signum == SIGINT)
        {
                if (pid == 0)
                {
                        printf("\n%s", prompt_str);
                        fflush(stdout);
                }
        }
        else if (signum == SIGTSTP)
        {
                if (pid == 0)
                {
                        printf("\n%s", prompt_str);
                        fflush(stdout);
                }
                else
                {
                        insert_first(&head, input_str, pid);
                }
        }
        else if (signum == SIGCHLD)
        {
                // clear the resources
                waitpid(-1, &status, WNOHANG);
        }
}
void scan_input(char *prompt_str, char *input_str)
{
        signal(SIGINT, signal_handler);
        signal(SIGTSTP, signal_handler);

        extract_external_commands(external_commands);

        while(1)
        {
                input_str[0] = '\0';
                printf("%s", prompt_str);
                fflush(stdout);
                // read input from user
                scanf("%[^\n]", input_str);
                __fpurge(stdin);
                // if empty string - continue
                if (input_str[0] == '\0')
                {
                        continue;
                }
                // check PS1 is passed or not
                if (strncmp(input_str, "PS1=", 4) == 0)
                {
                        // check if there is space (input_str[4])
                        if (strchr(input_str, ' '))
                        {
                                // if space
                                printf("PS1: command not found\n");
                        }
                        else
                        {
                                // no space
                                strncpy(prompt_str, input_str + 4, sizeof(prompt_str) - 1);
                        }
                }
                // if not PS1
                else
                {

                        char *cmd = get_command(input_str);

                        // identify ext/int command
                        int ret = check_command_type(cmd, external_commands);

                        if (ret == BUILTIN)
                        {
                                // execute internal commands
                                execute_internal_commands(input_str);
                        }
                        else if (ret == EXTERNAL)
                        {
                                // execute external commands
                                // create a child process
                                pid = fork();

                                if (pid > 0)
                                {
                                        // parent
                                        waitpid(pid, &status, WUNTRACED);
                                        pid = 0;
                                }
                                else if (pid == 0)
                                {
                                        // child
                                        signal(SIGINT, SIG_DFL);
                                        signal(SIGTSTP, SIG_DFL);
                                        execute_external_commands(input_str);
                                }
                        }
                        else
                        {
                                // print error
                                printf("Command \"%s\" not found\n", cmd);
                        }
                }

        }
}