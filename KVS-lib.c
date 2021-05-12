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

    if (sizeof(group_id) > 512 || sizeof(secret) > 512)
    {
        printf("Size of group and secret must be below 512 bytes.\n");
        return -5;
    }

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
    write(send_socket, group_id, size_group);
    write(send_socket, &size_secret, sizeof(size_secret));
    write(send_socket, secret, size_secret);

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
    int flag;

    if (sizeof(key) < 0) //ainda a definir
    {
        printf("Size of group and secret must be below 512 bytes.\n");
        return -5;
    }

    if (1 == 1) //verificar que a socket esta ligada ao server
    {
        char command[5] = "PUT_";
        int size_key = strlen(key);
        int size_value = strlen(value);
        write(send_socket, &command, sizeof(command));
        write(send_socket, &size_key, sizeof(size_key));
        write(send_socket, key, strlen(key));
        write(send_socket, &size_value, sizeof(size_value));
        write(send_socket, value, strlen(value));

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
    else
    {
        printf("You are not connected to any group.\n");
        return -100;
    }
}

int get_value(char *key, char **value)
{
    int flag, size_buffer;

    if (sizeof(key) < 0) //ainda a definir
    {
        printf("Size of group and secret must be below 512 bytes.\n");
        return -5;
    }

    if (1 == 1) //verificar que a socket esta ligada ao server
    {
        char command[5] = "GET_";
        int size_key = strlen(key);
        write(send_socket, &command, sizeof(command));
        write(send_socket, &size_key, sizeof(size_key));
        write(send_socket, key, strlen(key));

        int err_rcv = recv(send_socket, &flag, sizeof(int), 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }

        err_rcv = recv(send_socket, &size_buffer, sizeof(size_buffer), 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }
        char buffer[size_buffer + 1];
        err_rcv = recv(send_socket, buffer, size_buffer, 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }
        printf("Size %d e '%s'\n", size_buffer, buffer);
        //switch case para os erros
        if (flag == 1)
        {
            printf("ok\n");
            *value = (char *)malloc((size_buffer + 1) * sizeof(char));
            strcpy(*value, buffer);
            return 0;
        }
        else
        {
            remove(client_addr);
            printf("Connection refused. Pair group/secret is wrong\n");
            return -1;
        }
    }
    else
    {
        printf("You are not connected to any group.\n");
        return -100;
    }
}

int delete_value(char *key)
{
    int flag, size_buffer;
    char *buffer;

    if (sizeof(key) < 0) //ainda a definir
    {
        printf("Size of group and secret must be below 512 bytes.\n");
        return -5;
    }

    if (1 == 1) //verificar que a socket esta ligada ao server
    {
        char command[5] = "DEL_";
        int size_key = strlen(key);
        write(send_socket, &command, sizeof(command));
        write(send_socket, &size_key, sizeof(size_key));
        write(send_socket, key, strlen(key));

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
    else
    {
        printf("You are not connected to any group.\n");
        return -100;
    }
}

int register_callback(char *key, void (*callback_function)(char *))
{
}

int close_connection()
{
    int flag;
    if (1 == 1) //verificar que a socket esta ligada ao server
    {
        char command[5] = "CLS_";
        write(send_socket, &command, sizeof(command));

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
    else
    {
        printf("You are not connected to any group.\n");
        return -100;
    }
}