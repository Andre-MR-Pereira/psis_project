#include "hash.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define HASHSIZE 10001

int extract_command(char *packet, char *field1, char *field2)
{
    char *command, *f1, *f2;

    command = strtok(packet, "_");

    f1 = strtok(NULL, "_");
    if (f1 != NULL)
    {
        strcpy(field1, f1);
    }

    f2 = strtok(NULL, "_");
    if (f2 != NULL)
    {
        strcpy(field2, f2);
    }

    if (strcmp("CRE", command) == 0)
    {
        return 0;
    }
    else if (strcmp("DEL", command) == 0)
    {
        return 1;
    }
    else if (strcmp("CMP", command) == 0)
    {
        return 2;
    }
    else if (strcmp("ASK", command) == 0)
    {
        return 3;
    }
    return -1;
}

int main()
{
    int server_socket, n_bytes, flag;
    struct sockaddr_in server_socket_addr;
    struct sockaddr_in sender_sock_addr;
    socklen_t size_sender_addr;
    int sender_sock_addr_size = sizeof(sender_sock_addr);
    char command[5], field1[512], field2[512], buffer[1040];
    int size_field1, size_field2;
    hashtable *group;
    hashtable **vault;
    char fieldz[100];

    vault = allocate_table(HASHSIZE);

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1)
    {
        exit(-1);
    }

    inet_aton("192.168.1.73", &server_socket_addr.sin_addr);
    server_socket_addr.sin_port = htons(3001);
    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_socket_addr, sizeof(server_socket_addr)) == -1)
    {
        printf("Server socket already created\n");
        exit(-1);
    }

    while (1)
    {
        size_sender_addr = sizeof(struct sockaddr_storage);
        n_bytes = recvfrom(server_socket, &buffer, sizeof(buffer), 0,
                           (struct sockaddr *)&sender_sock_addr, &size_sender_addr);

        //printf("received %d byte (string %s) from %d\n", n_bytes, buffer, sender_sock_addr.sin_addr.s_addr);

        switch (extract_command(buffer, field1, field2))
        {
        case 0: //create
            flag = 1;

            //n_bytes = recvfrom(server_socket, field1, sizeof(field1), 0,
            //                   (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);
            //gerar aqui o secret
            //n_bytes = recvfrom(server_socket, field2, sizeof(field2), 0,
            //                   (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);
            //n_bytes = recvfrom(server_socket, field2, sizeof(field2), 0,
            //                   (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);
            printf("%s e %s do Local\n", field1, field2);
            //group = insert(vault, field1, secret, HASHSIZE);
            group = insert(vault, field1, field2, HASHSIZE);

            if (group == NULL) //the group already exists
            {
                flag = -4; //confirmar que este não é usado em mais lado nenhum, significa que o grupo já existe
            }

            //enviar apenas 1 buffer com a flag e o secret em caso de sucesso
            sendto(server_socket, &flag, sizeof(flag), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
            break;
        case 1: //delete
            flag = 1;
            /*n_bytes = recvfrom(server_socket, field1, sizeof(field1), 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);*/
            if (delete_hash(vault, field1, HASHSIZE) == -1)
            {
                printf("Hash deletion failed\n");
                flag = -3;
            }
            sendto(server_socket, &flag, sizeof(flag), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
            break;
        case 2: //compare
            /*n_bytes = recvfrom(server_socket, field1, sizeof(field1), 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);

            n_bytes = recvfrom(server_socket, field2, sizeof(field2), 0,
                               (struct sockaddr *)&sender_sock_addr, &sender_sock_addr_size);*/
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
            sendto(server_socket, &flag, sizeof(flag), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
            break;
        case 3: //ask for the secret of a certain group

            break;
        default:
            printf("Command not found.\n");
            flag = -2;
            sendto(server_socket, &flag, sizeof(flag), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
        }
    }
}