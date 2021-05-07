#include "KVS-lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <pthread.h>

#define SERVER_SOCKET_ADDR "/tmp/server_socket"

char client_addr[25];

int send_socket;

int establish_connection(char *group_id, char *secret)
{
    int flag;
    struct sockaddr_un server_socket_addr, socket_addr_client;

    pid_t pid = getpid();
    snprintf(client_addr, 1000, "/tmp/client_socket_%d", pid);
    printf("%s\n", client_addr);

    socket_addr_client.sun_family = AF_UNIX;

    strcpy(socket_addr_client.sun_path, client_addr);

    send_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (send_socket == -1)
    {
        exit(-1);
    }

    if (bind(send_socket, (struct sockaddr *)&socket_addr_client, sizeof(socket_addr_client)) == -1)
    {
        printf("Socket for client %d already created\n", pid);
        exit(-1);
    }

    strcpy(server_socket_addr.sun_path, SERVER_SOCKET_ADDR);
    server_socket_addr.sun_family = AF_UNIX;

    int err_c = connect(send_socket, (struct sockaddr *)&server_socket_addr, sizeof(server_socket_addr));
    if (err_c == -1)
    {
        perror("connect");
        exit(-1);
    }
    char command[5] = "EST_";
    int size_group = strlen(group_id);
    int size_secret = strlen(secret);
    write(send_socket, &command, sizeof(command));
    write(send_socket, &size_group, sizeof(size_group));
    write(send_socket, group_id, strlen(group_id));
    write(send_socket, &size_secret, sizeof(size_secret));
    write(send_socket, secret, strlen(secret));

    int err_rcv = recv(send_socket, &flag, sizeof(int), 0);
    if (err_rcv == -1)
    {
        perror("recieve");
        exit(-1);
    }

    //switch case para os erros
    if (flag == 1)
    {
        printf("ok\n");
        return 0;
    }
    else
    {
        remove(client_addr);
        printf("Connection refused. Pair group/secret is wrong\n");
        return -1;
    }
}

int put_value(char *key, char *value)
{
}

int get_value(char *key, char **value)
{
}

int delete_value(char *key)
{
}

int register_callback(char *key, void (*callback_function)(char *))
{
}

int close_connection()
{
}