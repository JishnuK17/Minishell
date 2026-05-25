#include "header.h"

void print_list(Slist *head)
{
        if (head != NULL)
        {
                while (head)
                {
                        printf("[%d]    ", head->pid);
                        printf("%s\n", head->cmd);
                        head = head->link;
                }
        }
}

void delete_first(Slist **head)
{
        if (*head == NULL)
                return;
        Slist *temp = *head;
        *head = (*head)->link;
        free(temp);
}

void insert_first(Slist **head, char *input_str, int pid)
{
        Slist *newnode;
        newnode = (Slist *)malloc(sizeof(Slist));

        if (!newnode)
                return;

        newnode->pid = pid;
        strcpy(newnode->cmd, input_str);
        newnode->link = NULL;

        newnode->link = *head;
        *head = newnode;

}