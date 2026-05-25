
#include<stdio.h>
#include "header.h"

char prompt_str[50] = "minishell$:";
char input_str[100];

int main()
{
    system("clear");
    scan_input(prompt_str, input_str);
    return 0;
}