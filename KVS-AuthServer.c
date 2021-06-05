#include "hash.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>

#define HASHSIZE 10001
pthread_rwlock_t groups_rwlock = PTHREAD_RWLOCK_INITIALIZER;

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

void assemble_payload(char *buffer, int flag, char *field1, char *field2)
{
    char flagstr[2];
    sprintf(flagstr, "%d", flag);
    strcpy(buffer, "");
    strcat(buffer, flagstr);
    strcat(buffer, "_");
    if (field1 != NULL)
    {
        strcat(buffer, field1);
        strcat(buffer, "_");
    }
    if (field2 != NULL)
    {
        strcat(buffer, field2);
        strcat(buffer, "_");
    }
}

//sets the provided buffer to '\0'
void cleanBuffer(char *buff)
{
    for (int i = 0; i < strlen(buff); i++)
    {
        buff[i] = '\0';
    }
}

char *generate_secret()
{

    char *secret = malloc(512 * sizeof(char));
    if (secret == NULL)
    {
        printf("erro a allocar segredo\n");
        //damos exit??  ou perror
    }
    //código random de geração do segredo
    strcpy(secret, "12345");

    return secret;
}

int main()
{
    int server_socket, n_bytes, flag;
    struct sockaddr_in server_socket_addr;
    struct sockaddr_in sender_sock_addr;
    socklen_t size_sender_addr;
    int sender_sock_addr_size = sizeof(sender_sock_addr);
    char command[5], field1[512], field2[512], prev_buffer[1040], buffer[1040], send_buffer[1040];
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

    inet_aton("172.22.146.84", &server_socket_addr.sin_addr); // André: "192.168.1.73"
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
        printf("+++++++++++++++++++++++++\n");
        printf("%s\n", buffer);
        switch (extract_command(buffer, field1, field2))
        {
        case 0: //create
            flag = 1;

            printf("%s e %s do Local\n", field1, field2);

            group = lookup(vault, field1, HASHSIZE);

            if (group == NULL) //the group doesn't exist yet
            {
                //generate secret
                strcpy(field2, generate_secret());

                if (pthread_rwlock_wrlock(&groups_rwlock) != 0)
                {
                    perror("Lock Put write lock failed");
                }
                group = insert(vault, field1, field2, HASHSIZE);
                if (pthread_rwlock_unlock(&groups_rwlock) != 0)
                {
                    perror("Unlock Put write lock failed");
                }

                if (group == NULL) //the group couldn´t be created
                {
                    flag = -3;
                    assemble_payload(send_buffer, flag, NULL, NULL);
                }
                else
                {
                    //we only need to send the flag and the secret
                    assemble_payload(send_buffer, flag, field1, field2);
                }
                printf("Nanda flag %d\n", flag);
                //enviar apenas 1 buffer com a flag e o secret em caso de sucesso
                printf("Sends %s\n", send_buffer);
                sendto(server_socket, &send_buffer, sizeof(send_buffer), 0,
                       (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
            }
            else //the group already exists
            {
                flag = -9;
                assemble_payload(send_buffer, flag, NULL, NULL);
                printf("Nanda flag %d\n", flag);
                printf("Sends %s\n", send_buffer);
                sendto(server_socket, &send_buffer, sizeof(send_buffer), 0,
                       (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
            }

            break;
        case 1: //delete
            flag = 1;

            if (pthread_rwlock_wrlock(&groups_rwlock) != 0)
            {
                perror("Lock Put write lock failed");
            }
            if (delete_hash(vault, field1, HASHSIZE) == -1)
            {
                printf("Hash deletion failed\n");
                flag = -3;
            }
            if (pthread_rwlock_unlock(&groups_rwlock) != 0)
            {
                perror("Unlock Put write lock failed");
            }

            assemble_payload(send_buffer, flag, NULL, NULL);
            printf("Nanda flag %d\n", flag);
            printf("Sends %s\n", send_buffer);
            sendto(server_socket, &send_buffer, sizeof(send_buffer), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
            break;
        case 2: //compare

            if (pthread_rwlock_rdlock(&groups_rwlock) != 0)
            {
                perror("Lock Put write lock failed");
            }
            group = lookup(vault, field1, HASHSIZE);
            if (pthread_rwlock_unlock(&groups_rwlock) != 0)
            {
                perror("Unlock Put write lock failed");
            }

            if (group != NULL && strcmp(group->value, field2) == 0) //correto
            {
                flag = 1;
                printf("Checks out\n");
                assemble_payload(send_buffer, flag, field1, field2);
            }
            else if (group == NULL)
            {
                flag = -2;
                printf("Group not found\n");
                assemble_payload(send_buffer, flag, NULL, NULL);
            }
            else
            {
                flag = -3;
                printf("Combination was off\n");
                assemble_payload(send_buffer, flag, NULL, NULL);
            }
            printf("Nanda flag %d\n", flag);
            printf("Sends %s\n", send_buffer);
            sendto(server_socket, &send_buffer, sizeof(send_buffer), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
            break;
        case 3: //ask for the secret of a certain group

            if (pthread_rwlock_rdlock(&groups_rwlock) != 0)
            {
                perror("Lock Put write lock failed");
            }
            group = lookup(vault, field1, HASHSIZE);
            if (pthread_rwlock_unlock(&groups_rwlock) != 0)
            {
                perror("Unlock Put write lock failed");
            }

            if (group != NULL) //correto
            {
                flag = 1;
                printf("Sending secret\n");
                assemble_payload(send_buffer, flag, field1, group->value);
            }
            else
            {
                flag = -2;
                printf("Group not found\n");
                assemble_payload(send_buffer, flag, NULL, NULL);
            }
            printf("Nanda flag %d\n", flag);
            printf("Sends %s\n", send_buffer);
            sendto(server_socket, &send_buffer, sizeof(send_buffer), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
            break;
        default:
            printf("Command not found.\n");
            flag = -404;
            sendto(server_socket, &flag, sizeof(flag), 0,
                   (struct sockaddr *)&sender_sock_addr, sender_sock_addr_size);
        }
        strcpy(prev_buffer,buffer);
        cleanBuffer(buffer);
        cleanBuffer(send_buffer);
    }
}