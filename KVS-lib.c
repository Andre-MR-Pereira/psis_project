#include "KVS-lib.h"
#include "KVS-libmod.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <pthread.h>

int establish_connection(char *group_id, char *secret)
{
    int flag = 0;
    struct sockaddr_un server_socket_addr, socket_addr_client;

    if (sizeof(group_id) > 512 || sizeof(secret) > 512)
    {
        printf("Size of group and secret must be below 512 bytes.\n");
        return -5;
    }
    if (group_id == NULL || secret == NULL)
        return -500;

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
        remove(client_addr);
        exit(-1);
    }
    char command[5] = "EST_";
    int size_group = strlen(group_id) + 1;
    int size_secret = strlen(secret) + 1;
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
        hooked = 1;
        printf("You established connection with the group %s\n", group_id);
        return flag;
    }
    else if (flag == -4)
    {
        printf("Establish failed with the group %s,so you will remain connected to your previous group\n", group_id);
        return flag;
    }
    else if(flag == -7) 
    {
        printf("Establish failed with the group %s, group is deleted. Not connected to any group\n", group_id);
        return flag;
    }
    else
    {
        remove(client_addr);
        printf("Establish failed: Not connected to any group\n");
        return flag;
    }
}

int put_value(char *key, char *value)
{
    int flag = 0;

    if (sizeof(key) < 0) //ainda a definir
    {
        printf("Size of group and secret must be below 512 bytes.\n");
        return -11;
    }
    if (key == NULL || value == NULL)
        return -500;

    if (hooked == 1) //verificar que a socket esta ligada ao server
    {
        char command[5] = "PUT_";
        int size_key = strlen(key) + 1;
        int size_value = strlen(value) + 1;
        write(send_socket, &command, sizeof(command));
        write(send_socket, &size_key, sizeof(size_key));
        write(send_socket, key, size_key);
        write(send_socket, &size_value, sizeof(size_value));
        write(send_socket, value, size_value);

        int err_rcv = recv(send_socket, &flag, sizeof(int), 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }

        //switch case para os erros
        if (flag == 1)
        {
            printf("The Key/Value has been placed\n");
            return flag;
        }
        else if (flag == -3)
        {
            printf("Put failed\n");
            return flag;
        }
        else
        {
            printf("Put: Unknown flag\n");
            return -6;
        }
    }
    else
    {
        printf("You are not connected to any group.\n");
        return -1;
    }
}

int get_value(char *key, char **value)
{
    int flag = 0, size_buffer = 0;

    if (sizeof(key) < 0) //ainda a definir
    {
        printf("Size of group and secret must be below 512 bytes.\n");
        return -11;
    }
    if (key == NULL || value == NULL)
        return -500;

    if (hooked == 1) //verificar que a socket esta ligada ao server
    {
        char command[5] = "GET_";
        int size_key = strlen(key) + 1;
        write(send_socket, &command, sizeof(command));
        write(send_socket, &size_key, sizeof(size_key));
        write(send_socket, key, size_key);

        int err_rcv = recv(send_socket, &flag, sizeof(int), 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }
        if (flag == 1)
        {
            err_rcv = recv(send_socket, &size_buffer, sizeof(size_buffer), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            char buffer[size_buffer];
            err_rcv = recv(send_socket, buffer, size_buffer, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            printf("Value has been fetched\n");
            *value = (char *)malloc((size_buffer) * sizeof(char));
            strcpy(*value, buffer);
            return flag;
        }
        else if (flag == -2)
        {
            printf("GET | Key not found\n");
            return flag;
        }
        else
        {
            printf("Get: Unknown flag\n");
            return -6;
        }
    }
    else
    {
        printf("You are not connected to any group.\n");
        return -1;
    }
}

int delete_value(char *key)
{
    int flag = 0, size_buffer;
    char *buffer;

    if (sizeof(key) < 0) //ainda a definir
    {
        printf("Size of group and secret must be below 512 bytes.\n");
        return -11;
    }
    if (key == NULL)
        return -500;

    if (hooked == 1) //verificar que a socket esta ligada ao server
    {
        char command[5] = "DEL_";
        int size_key = strlen(key) + 1;
        write(send_socket, &command, sizeof(command));
        write(send_socket, &size_key, sizeof(size_key));
        write(send_socket, key, size_key);

        int err_rcv = recv(send_socket, &flag, sizeof(int), 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }

        if (flag == 1)
        {
            printf("Key entry %s has been deleted\n", key);
            return flag;
        }
        else if (flag == -2)
        {
            printf("DEL | Key not found\n");
            return flag;
        }
        else if (flag == -3)
        {
            printf("Delete failed\n");
            return flag;
        }
        else
        {
            printf("Del: Unknown flag\n");
            return -6;
        }
    }
    else
    {
        printf("You are not connected to any group.\n");
        return -1;
    }
}

int register_callback(char *key, void (*callback_function)(char *))
{
    int flag = 0, err_rcv, trigger;
    char *buffer;
    int plug;
    char *plug_addr;
    struct sockaddr_un callback_addr_server, callback_addr_client;

    if (key == NULL)
        return -500;

    if (hooked == 1) //verificar que a socket esta ligada ao server
    {

        callback_addr_client.sun_family = AF_UNIX;
        plug_addr = (char *)malloc((strlen(client_addr) + strlen(key) + 1) * sizeof(char));

        strcpy(plug_addr, client_addr);
        strcat(plug_addr, key);
        strcpy(callback_addr_client.sun_path, plug_addr);
        free(plug_addr);

        plug = socket(AF_UNIX, SOCK_STREAM, 0);
        if (plug == -1)
        {
            exit(-1);
        }

        if (bind(plug, (struct sockaddr *)&callback_addr_client, sizeof(callback_addr_client)) == -1)
        {
            printf("CallbackSocket for key %s already created\n", key);
            return -69;
        }

        strcpy(callback_addr_server.sun_path, CALLBACK_SOCKET_ADDR);
        callback_addr_server.sun_family = AF_UNIX;

        if (fork() != 0)
        {
            int err_c = connect(plug, (struct sockaddr *)&callback_addr_server, sizeof(callback_addr_server));
            if (err_c == -1)
            {
                perror("connect");
                exit(-1);
            }
            while (1)
            {
                err_rcv = recv(plug, &trigger, sizeof(trigger), 0);
                if (err_rcv == -1)
                {
                    perror("recieve");
                    exit(-1);
                }
                else if (err_rcv == 0)
                {
                    printf("Closing %s callback pipe (Local went down)\n", key);
                    remove(callback_addr_client.sun_path);
                    exit(EXIT_SUCCESS);
                }

                if (trigger == 100)
                {
                    callback_function(key);
                }
                else if (trigger == -5)
                {
                    printf("Closing %s callback pipe\n", key);
                    remove(callback_addr_client.sun_path);
                    exit(EXIT_SUCCESS);
                }
            }
        }
        else
        {
            char command[5] = "RCL_";
            int size_key = strlen(key) + 1;
            write(send_socket, &command, sizeof(command));
            write(send_socket, &size_key, sizeof(size_key));
            write(send_socket, key, size_key);

            err_rcv = recv(send_socket, &flag, sizeof(int), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }

            if (flag == 1)
            {
                printf("Callback function for %s has been mounted\n", key);
                return flag;
            }
            else if (flag == -2)
            {
                printf("RCL | Key not found\n");
                return flag;
            }
            else if (flag == -10)
            {
                printf("Couldn´t save the callback socket\n");
                return flag;
            }
            else
            {
                printf("Rcl: Unknown flag\n");
                return -6;
            }
        }
    }
    else
    {
        printf("You are not connected to any group.\n");
        return -1;
    }
}

int close_connection()
{
    int flag = 0;
    if (hooked == 1) //verificar que a socket esta ligada ao server
    {
        char command[5] = "CLS_";
        write(send_socket, &command, sizeof(command));

        int err_rcv = recv(send_socket, &flag, sizeof(int), 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }

        if (flag == 1)
        {
            printf("Connection closed\n");
            hooked = 0;
            remove(client_addr);
            return flag;
        }
        else
        {
            printf("Close failed\n");
            return -6;
        }
    }
    else
    {
        printf("You are not connected to any group.\n");
        return -1;
    }
}