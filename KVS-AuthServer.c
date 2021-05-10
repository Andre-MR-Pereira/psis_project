#include "hash.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>

#define HASHSIZE 1001
#define SERVER_SOCKET_ADDR "/tmp/auth_socket"

int extract_command(char *command)
{
    char *token = strtok(command, "_");
    if (strcmp("CRE", token) == 0)
    {
        return 0;
    }
    else if (strcmp("DEL", token) == 0)
    {
        return 1;
    }
    else if (strcmp("CMP", token) == 0)
    {
        return 2;
    }
    return -1;
}

int main()
{
    int server_socket, n_bytes, flag;
    struct sockaddr_un server_socket_addr;
    struct sockaddr_un sender_sock_addr;
    int sender_sock_addr_size = sizeof(sender_sock_addr);
    char command[5], *field1, *field2;
    int size_field1, size_field2;
    hashtable *group;
    hashtable **vault;
    vault = allocate_table(HASHSIZE);
    char fieldz[100];
    remove(SERVER_SOCKET_ADDR);

    server_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (server_socket == -1)
    {
        exit(-1);
    }

    server_socket_addr.sun_family = AF_UNIX;
    strcpy(server_socket_addr.sun_path, SERVER_SOCKET_ADDR);

    if (bind(server_socket, (struct sockaddr *)&server_socket_addr, sizeof(server_socket_addr)) == -1)
    {
        printf("Server socket already created\n");
        exit(-1);
    }

    //fork();
    //fork();
    while (1)
    {
        n_bytes = recvfrom(server_socket, &command, sizeof(command), 0,
                           (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);

        printf("received %d byte (string %s) from %s\n", n_bytes, command, sender_sock_addr.sun_path);

        switch (extract_command(command))
        {
        case 0:
            flag = 1;

            n_bytes = recvfrom(server_socket, &size_field1, sizeof(size_field1), 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);

            field1 = (char *)malloc((size_field1 + 1) * sizeof(char));
            n_bytes = recvfrom(server_socket, field1, size_field1, 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);

            n_bytes = recvfrom(server_socket, &size_field2, sizeof(size_field2), 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);
            field2 = (char *)malloc((size_field2 + 1) * sizeof(char));
            n_bytes = recvfrom(server_socket, field2, size_field2, 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);
            group = insert(vault, field1, field2, HASHSIZE);
            free(field1);
            free(field2);
            sendto(server_socket, &flag, sizeof(flag), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
            break;
        case 1:
            flag = 1;
            n_bytes = recvfrom(server_socket, &size_field1, sizeof(size_field1), 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);
            field1 = (char *)malloc((size_field1 + 1) * sizeof(char));
            n_bytes = recvfrom(server_socket, field1, size_field1, 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);
            if (delete_hash(vault, field1, HASHSIZE) == -1)
            {
                printf("Hash deletion failed\n");
                flag = -3;
            }
            free(field1);
            sendto(server_socket, &flag, sizeof(flag), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
            break;
        case 2:
            n_bytes = recvfrom(server_socket, &size_field1, sizeof(size_field1), 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);
            field1 = (char *)malloc((size_field1 + 1) * sizeof(char));
            n_bytes = recvfrom(server_socket, field1, size_field1, 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);

            n_bytes = recvfrom(server_socket, &size_field2, sizeof(size_field2), 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);
            field2 = (char *)malloc((size_field2 + 1) * sizeof(char));
            n_bytes = recvfrom(server_socket, field2, size_field2, 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);
            group = lookup(vault, field1, HASHSIZE);
            if (strcmp(group->value, field2) == 0) //correto
            {
                flag = 1;
                printf("Checks out\n");
            }
            else
            {
                flag = -1;
                printf("Combination was off\n");
            }
            free(field1);
            free(field2);
            sendto(server_socket, &flag, sizeof(flag), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
            break;
        default:
            printf("Command not found.\n");
            flag = -2;
            sendto(server_socket, &flag, sizeof(flag), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
        }
    }
}