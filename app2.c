#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <pthread.h>
#include "hash.h"
#include "KVS-lib.h"

#define SERVER_SOCKET_ADDR "/tmp/server_socket"

void *user_input(void *args)
{
    char buffer[1040];
    char *buff;
    int *server_buffer = (int *)args;
    char option[10] = "\0", input[520] = "\0", group_name[512] = "\n", secret[512] = "\n",value[512] = "\n",key[512] = "\n"; 
    //na realidade o value não é limitado, mas para fins de teste aqui tem o limite de 512
    int send_socket = *server_buffer;
    while (1)
    {
         

        if (fgets(input, 520, stdin) == NULL)
        {
            perror("Sentence");
            exit(-1);
        }

        if (!sscanf(input, " %s", option))
            exit(-1);

        if(strcmp(option,"establish")==0)
        {
            if (sscanf(input, " %s %s %s", option, group_name, secret) == 3)
            {
                establish_connection(group_name,secret);
            }
        }
        else if(strcmp(option,"put")==0)
        {
            if (sscanf(input, " %s %s %s", option, key,value) == 3)
            {
                put_value(key,value);
            }
        }
        else if(strcmp(option,"get")==0)
        {
            if (sscanf(input, " %s %s", option, key) == 2)
            {
                printf("get value\n");
                get_value(key,&buff);
                printf("value %s\n",buff);
            }
        }
        else if(strcmp(option,"del")==0)
        {
            if (sscanf(input, " %s %s", option, key) == 2)
            {
                delete_value(key);
            }
        }
        else if(strcmp(option,"register")==0)
        {
            if (sscanf(input, " %s %s %s", option, key,value) == 3)
            {
               printf("não pus aqui a chamada\n");
            }
        }
        else if(strcmp(option,"close")==0)
        {
            
            close_connection();
            
        }
        else
            printf("not a command, vai dar bosta\n");

        //write(send_socket, &buffer, sizeof(buffer));
    }
}

void *listener(void *args)
{
    int *server_buffer = (int *)args;
    int send_socket = *server_buffer;
    char buffer[100], name[50];
    int size_name, buffer_size;
    while (1)
    {
        int err_rcv = recv(send_socket, &size_name, sizeof(size_name), 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }
        err_rcv = recv(send_socket, &name, size_name, 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }
        err_rcv = recv(send_socket, &buffer_size, sizeof(buffer_size), 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }
        err_rcv = recv(send_socket, &buffer, buffer_size, 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }
        buffer[buffer_size] = '\0';
        printf("%s said:%s\n", name, buffer);
    }
}

int main()
{
    char client_addr[25];
    int send_socket, buffer;
    struct sockaddr_un server_socket_addr, socket_addr_client;
    pthread_t t_id1, t_id2;

    srand(time(NULL));

    pid_t pid = getpid();
    snprintf(client_addr, 1000, "/tmp/client_socket_%d", pid);
    printf("%s\n", client_addr);

    /*socket_addr_client.sun_family = AF_UNIX;

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
    }*/

    if (pthread_create(&t_id1, NULL, user_input, (void *)&send_socket) != 0)
    {
        printf("Error on thread creation");
    }
    /*if (pthread_create(&t_id2, NULL, listener, (void *)&send_socket) != 0)
    {
        printf("Error on thread creation");
    }*/
    pthread_join(t_id1, 0);
    //pthread_join(t_id2, 0);

    return 0;
}