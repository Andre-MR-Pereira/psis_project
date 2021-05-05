#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <pthread.h>
#include "hash.h"

#define SERVER_SOCKET_ADDR "/tmp/server_socket"

typedef struct client_list
{
    int pid;
    struct sockaddr_un client_socket_addr;
    time_t connection_open;
    time_t connection_close;
    struct client_list *next;
} client_list;

typedef struct client_list
{
    char *group;
    hashtable **group_table;
    struct client_list *next;
} hash_list;

int client_fd_vector[10];
int accepted_connections = 0;
//fazer uma hash de clientes?
//client_list *head_connections[10];

void *client_interaction(void *args)
{
    client_list *aux;
    char group_id[100], secret[100];
    int *client_buffer = (int *)args;
    int client_fd = *client_buffer;
    int toset = 1;
    int size_group, size_secret, buffer_size;
    int pos_connects;

    /*for (int i = 0; i < accepted_connections; i++)
    {
        if (client_fd_vector[i] == client_fd)
        {
            pos_connects = i;
            head_connections[pos_connects].pid = extract_pid(client_fd_vector[i]);
        }
    }*/
    //race condition

    while (1)
    {
        int err_rcv = recv(client_fd, &size_group, sizeof(size_group), 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }
        err_rcv = recv(client_fd, &size_secret, sizeof(size_secret), 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }
        err_rcv = recv(client_fd, &group_id, size_group, 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }
        err_rcv = recv(client_fd, &secret, size_secret, 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }

        if (1 == 1 /*verificar com o auth server*/)
        {
            int connection_flag = 1;
            printf("%s e %s fornecidos\n", group_id, secret);
            write(client_fd, &connection_flag, sizeof(connection_flag));
        }
        else
        {
            int error_flag = -1;
            //guardar tempo de saida
            write(client_fd, &error_flag, sizeof(error_flag));
            pthread_exit(NULL);
        }
    }
}

int extract_pid(struct sockaddr_un sender_sock_addr)
{
    int pid;
    char *token = strtok(sender_sock_addr.sun_path, "_");
    token = strtok(NULL, "_");
    token = strtok(NULL, "_");
    pid = atoi(token);
    return pid;
}

int main()
{
    int server_socket, buffer;
    struct sockaddr_un server_socket_addr, client_socket_addr;
    pthread_t t_id[10];

    remove(SERVER_SOCKET_ADDR);

    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
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

    if (listen(server_socket, 2) == -1)
    {
        perror("listen");
        exit(-1);
    }
    int i = 0;
    int client_size = sizeof(client_socket_addr);
    while (1)
    {
        int client_fd = accept(server_socket, (struct sockaddr *)&client_socket_addr, &client_size);
        if (client_fd == -1)
        {
            perror("accept");
            exit(-1);
        }
        client_fd_vector[i] = client_fd;
        accepted_connections++;
        //guardar tempo conexão
        //guardar pid da pessoa (funcao ja esta criada)
        if (pthread_create(&t_id[i], NULL, client_interaction, (void *)&client_fd) != 0)
        {
            printf("Error on thread creation");
        }
        i++;
    }
}