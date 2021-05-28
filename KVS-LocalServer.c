#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "hash.h"

#define HASHSIZE 10001
#define SERVER_SOCKET_ADDR "/tmp/server_socket"
#define CALLBACK_SOCKET_ADDR "/tmp/callback_socket"
#define AUTH_SOCKET_ADDR "192.168.1.73"

pthread_rwlock_t groups_rwlock = PTHREAD_RWLOCK_INITIALIZER;

typedef struct client_list
{
    int pid;
    //struct sockaddr_un client_socket_addr;
    time_t connection_open;
    time_t connection_close;
    struct client_list *next;
} client_list;

typedef struct hash_list
{
    char *group;
    hashtable **group_table;
    pthread_rwlock_t hash_rwlock;
    int active_users;
    int remove_flag;
    struct hash_list *next;
} hash_list;

int client_fd_vector[1000];
int accepted_connections = 0;
int auth_server_fd = 0;
//client_list *head_connections[10];
hash_list *groups = NULL;
client_list *clients = NULL;
int send_socket;
int callback_socket;

int extract_command(char *command)
{
    char *token = strtok(command, "_");
    if (strcmp("EST", token) == 0)
    {
        return 0;
    }
    else if (strcmp("PUT", token) == 0)
    {
        return 1;
    }
    else if (strcmp("GET", token) == 0)
    {
        return 2;
    }
    else if (strcmp("DEL", token) == 0)
    {
        return 3;
    }
    else if (strcmp("RCL", token) == 0)
    {
        return 4;
    }
    else if (strcmp("CLS", token) == 0)
    {
        return 5;
    }
    return -1;
}

int extract_auth(char *packet, char *field1, char *field2)
{
    char *command, *f1, *f2;
    char buffer[2];
    int flag;

    strcpy(buffer, strtok(packet, "_"));
    flag = buffer[0] - '0';

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
    return flag;
}

void assemble_payload(char *buffer, char *command, char *field1, char *field2, int n_fields)
{
    strcpy(buffer, "");
    strcat(buffer, command);
    if (field1 != NULL)
    {
        strcat(buffer, field1);
        strcat(buffer, "_");
    }
    if (n_fields == 2)
    {
        if (field2 != NULL)
        {
            strcat(buffer, field2);
            strcat(buffer, "_");
        }
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

int extract_pid(struct sockaddr_un sender_sock_addr)
{
    int pid;
    char *token = strtok(sender_sock_addr.sun_path, "_");
    token = strtok(NULL, "_");
    token = strtok(NULL, "_");
    pid = atoi(token);
    return pid;
}

hash_list *lookup_group(char *group)
{
    hash_list *aux;
    for (aux = groups; aux != NULL; aux = aux->next)
    {
        if (strcmp(group, aux->group) == 0)
            return aux; /* found */
    }
    return NULL; /* not found */
}

void show_groups()
{
    hash_list *aux;
    for (aux = groups; aux != NULL; aux = aux->next)
    {
        printf("%s|", aux->group);
    }
    printf("\n");
}

void create_new_group(char *group)
{
    hash_list *aux;

    aux = (hash_list *)malloc(sizeof(hash_list));
    aux->group = strdup(group);
    aux->group_table = allocate_table(HASHSIZE);
    aux->active_users = 0;
    aux->remove_flag = 0;
    if (pthread_rwlock_init(&(aux->hash_rwlock), NULL) != 0)
    {
        perror("Error creating a hash lock");
    }
    if (groups == NULL)
    {
        aux->next = NULL;
    }
    else
    {
        aux->next = groups;
    }
    groups = aux;
}

void delete_group(char *group)
{
}

void *client_interaction(void *args)
{
    client_list *aux;
    hash_list *group;
    hashtable *buffer;
    int *client_buffer = (int *)args;
    int client_fd = *client_buffer;
    int size_field1, size_field2, hooked = 0;
    int connection_flag, error_flag, n_bytes;
    char command[5], field1[512], field2[512], auth_command[5], *value, auth_buffer[1040];
    struct sockaddr_in other_sock_addr;
    struct sockaddr_un client_callback_addr;

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
        int err_rcv = recv(client_fd, &command, sizeof(command), 0);
        if (err_rcv == -1)
        {
            perror("recieve");
            exit(-1);
        }
        else if (err_rcv == 0)
        {
            printf("Closing connection with client %s\n", ":Porque ele acabou a main");
            //guardar tempo de saida
            pthread_exit(NULL);
        }
        printf("   %d                    RECEBI COMMAND|  %s\n", err_rcv, command);
        memset(field1, 0, sizeof(field1));
        memset(field2, 0, sizeof(field2));

        switch (extract_command(command))
        {
        case 0: //establish connection
            err_rcv = recv(client_fd, &size_field1, sizeof(size_field1), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            err_rcv = recv(client_fd, field1, size_field1, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            err_rcv = recv(client_fd, &size_field2, sizeof(size_field2), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            err_rcv = recv(client_fd, field2, size_field2, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            printf("EST:Recebi %s e %s\n", field1, field2);

            other_sock_addr.sin_family = AF_INET;
            inet_aton(AUTH_SOCKET_ADDR, &other_sock_addr.sin_addr);
            other_sock_addr.sin_port = htons(3001);

            strcpy(auth_command, "CRE_");
            assemble_payload(auth_buffer, auth_command, field1, field2, 2);

            sendto(send_socket, &auth_buffer, sizeof(auth_buffer), 0,
                   (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));

            n_bytes = recvfrom(send_socket, &auth_buffer, sizeof(auth_buffer), 0,
                               NULL, NULL);
            connection_flag = extract_auth(auth_buffer, field1, field2);

            cleanBuffer(auth_buffer); //só para não ficar tudo bugado, limpa-se o buffer antes de mandar mais cenas

            strcpy(auth_command, "CMP_");
            assemble_payload(auth_buffer, auth_command, field1, field2, 2);
            sendto(send_socket, &auth_buffer, sizeof(auth_buffer), 0,
                   (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));

            n_bytes = recvfrom(send_socket, &auth_buffer, sizeof(auth_buffer), 0,
                               NULL, NULL);
            connection_flag = extract_auth(auth_buffer, field1, field2);

            if (connection_flag == 1) //verificar com o auth server
            {
                if (lookup_group(field1) == NULL)
                {
                    create_new_group(field1);
                }
                if (pthread_rwlock_rdlock(&groups_rwlock) != 0)
                {
                    perror("Lock EST write lock failed");
                }
                group = lookup_group(field1);
                if (pthread_rwlock_unlock(&groups_rwlock) != 0)
                {
                    perror("Unlock EST write lock failed");
                }
                group->active_users++;
                show_groups();
                connection_flag = 1;
                hooked = 1;
                write(client_fd, &connection_flag, sizeof(connection_flag));
            }
            else
            {
                if (hooked == 1)
                {
                    error_flag = -4;
                }
                else
                {
                    error_flag = -1;
                    //guardar tempo de saida
                }
                write(client_fd, &error_flag, sizeof(error_flag));
                pthread_exit(NULL);
            }
            break;
        case 1: //put_value
            err_rcv = recv(client_fd, &size_field1, sizeof(size_field1), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            err_rcv = recv(client_fd, field1, size_field1, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            err_rcv = recv(client_fd, &size_field2, sizeof(size_field2), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            value = (char *)malloc((size_field2 + 1) * sizeof(char));
            err_rcv = recv(client_fd, value, size_field2, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            printf("PUT:Recebi %s e %s\n", field1, value);
            if (hooked == 1)
            {
                if (pthread_rwlock_wrlock(&group->hash_rwlock) != 0)
                {
                    perror("Lock Put write lock failed");
                }
                if (group != NULL && (buffer = insert(group->group_table, field1, value, HASHSIZE)) != NULL)
                {
                    if (pthread_rwlock_unlock(&group->hash_rwlock) != 0)
                    {
                        perror("Unlock Put write lock failed");
                    }

                    /*if (pthread_rwlock_rdlock(&group->hash_rwlock) != 0)
                    {
                        perror("Lock Put write lock failed");
                    }
                    buffer = lookup(group->group_table, field1, HASHSIZE);
                    if (pthread_rwlock_unlock(&group->hash_rwlock) != 0)
                    {
                        perror("Unlock Put write lock failed");
                    }*/

                    callbacks *caux = buffer->head;
                    connection_flag = 100;
                    while (caux != NULL)
                    {
                        write(caux->callback_socket, &connection_flag, sizeof(connection_flag));
                        caux = caux->next;
                    }

                    connection_flag = 1;
                    write(client_fd, &connection_flag, sizeof(connection_flag));
                }
                else //cuidado com else que o insert pode ser NULL
                {
                    if (pthread_rwlock_unlock(&group->hash_rwlock) != 0)
                    {
                        perror("Unlock Put write lock failed");
                    }

                    error_flag = -3;
                    write(client_fd, &error_flag, sizeof(error_flag));
                }
                //memset(value, 0, sizeof(value));
                cleanBuffer(auth_buffer);

                free(value);
            }
            else
            {
                printf("You haven´t established a connection to a group yet.\n");
                error_flag = -1;
                group->active_users--;
                write(client_fd, &error_flag, sizeof(error_flag));
                //memset(value, 0, sizeof(value));
                cleanBuffer(auth_buffer);
                free(value);
                pthread_exit(NULL);
            }

            break;
        case 2: //get_value
            err_rcv = recv(client_fd, &size_field1, sizeof(size_field1), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            err_rcv = recv(client_fd, field1, size_field1, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }

            printf("GET:Recebi %s\n", field1);
            if (hooked == 1)
            {
                if (pthread_rwlock_rdlock(&group->hash_rwlock) != 0)
                {
                    perror("Lock GET write lock failed");
                }

                if (group != NULL && (buffer = lookup(group->group_table, field1, HASHSIZE)) != NULL)
                {
                    if (pthread_rwlock_unlock(&group->hash_rwlock) != 0)
                    {
                        perror("Unlock GET write lock failed");
                    }
                    connection_flag = 1;
                    write(client_fd, &connection_flag, sizeof(connection_flag));
                    int size_buffer = strlen(buffer->value);
                    printf("Size %d e '%s'\n", size_buffer, buffer->value);
                    write(client_fd, &size_buffer, sizeof(size_buffer));
                    write(client_fd, buffer->value, strlen(buffer->value));
                }
                else
                {
                    if (pthread_rwlock_unlock(&group->hash_rwlock) != 0)
                    {
                        perror("Unlock GET write lock failed");
                    }
                    error_flag = -2;
                    write(client_fd, &error_flag, sizeof(error_flag));
                }
            }
            else
            {
                printf("You haven´t established a connection to a group yet.\n");
                error_flag = -1;
                group->active_users--;

                write(client_fd, &error_flag, sizeof(error_flag));
                free(value);
                pthread_exit(NULL);
            }

            break;
        case 3: //delete_value
            err_rcv = recv(client_fd, &size_field1, sizeof(size_field1), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            err_rcv = recv(client_fd, field1, size_field1, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }

            printf("DEL:Recebi %s\n", field1);
            if (hooked == 1)
            {
                if (pthread_rwlock_rdlock(&group->hash_rwlock) != 0)
                {
                    perror("Lock DEL write lock failed");
                }

                if (group != NULL && (buffer = lookup(group->group_table, field1, HASHSIZE)) == NULL) //verificar com o auth server
                {
                    if (pthread_rwlock_unlock(&group->hash_rwlock) != 0)
                    {
                        perror("Unlock DEL write lock failed");
                    }

                    error_flag = -2;
                    write(client_fd, &error_flag, sizeof(error_flag));
                }
                else
                {
                    if (pthread_rwlock_unlock(&group->hash_rwlock) != 0)
                    {
                        perror("Unlock DEL write lock failed");
                    }

                    callbacks *caux = buffer->head;
                    connection_flag = -5;
                    while (caux != NULL)
                    {
                        write(caux->callback_socket, &connection_flag, sizeof(connection_flag));
                        caux = caux->next;
                    }

                    if (pthread_rwlock_wrlock(&group->hash_rwlock) != 0)
                    {
                        perror("Lock DEL write lock failed");
                    }
                    if (delete_hash(group->group_table, field1, HASHSIZE) == -1)
                    {
                        if (pthread_rwlock_unlock(&group->hash_rwlock) != 0)
                        {
                            perror("Unlock DEL write lock failed");
                        }
                        error_flag = -3;
                        write(client_fd, &error_flag, sizeof(error_flag));
                    }
                    else
                    {
                        if (pthread_rwlock_unlock(&group->hash_rwlock) != 0)
                        {
                            perror("Unlock DEL write lock failed");
                        }
                        connection_flag = 1;
                        write(client_fd, &connection_flag, sizeof(connection_flag));
                    }
                }
            }
            else
            {
                printf("You haven´t established a connection to a group yet.\n");
                error_flag = -1;
                group->active_users--;

                write(client_fd, &error_flag, sizeof(error_flag));
                free(value);
                pthread_exit(NULL);
            }

            break;
        case 4: //register_callback
            err_rcv = recv(client_fd, &size_field1, sizeof(size_field1), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            err_rcv = recv(client_fd, field1, size_field1, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }

            printf("RCL:Recebi %s\n", field1);
            if (hooked == 1)
            {
                if (pthread_rwlock_rdlock(&group->hash_rwlock) != 0)
                {
                    perror("Lock DEL write lock failed");
                }

                if (group != NULL && (buffer = lookup(group->group_table, field1, HASHSIZE)) != NULL)
                {
                    if (pthread_rwlock_unlock(&group->hash_rwlock) != 0)
                    {
                        perror("Unlock DEL write lock failed");
                    }

                    int callback_size = sizeof(client_callback_addr);
                    int callback_fd = accept(callback_socket, (struct sockaddr *)&client_callback_addr, &callback_size);
                    if (callback_fd == -1)
                    {
                        perror("accept");
                        exit(-1);
                    }

                    if (insert_callsocket(buffer, callback_fd) == 0)
                    {
                        connection_flag = 1;
                        write(client_fd, &connection_flag, sizeof(connection_flag));
                    }
                    else
                    {
                        error_flag = -10;
                        write(client_fd, &error_flag, sizeof(error_flag));
                    }
                }
                else
                {
                    error_flag = -2;
                    write(client_fd, &error_flag, sizeof(error_flag));
                }
            }
            else
            {
                printf("You haven´t established a connection to a group yet.\n");
                error_flag = -1;
                group->active_users--;
                write(client_fd, &error_flag, sizeof(error_flag));
                free(value);
                pthread_exit(NULL);
            }

            break;
        case 5: //close_connection
            printf("Closing connection with client\n");
            hooked = 0;
            group->active_users--;
            connection_flag = 1;
            write(client_fd, &connection_flag, sizeof(connection_flag));
            //guardar tempo de saida
            pthread_exit(NULL);
            break;
        default:
            printf("Command not found. Connection with client %d dropped\n", client_fd);
            hooked = 0;
            group->active_users--;
            error_flag = -404;
            //guardar tempo de saida
            write(client_fd, &error_flag, sizeof(error_flag));
            pthread_exit(NULL);
        }
    }
}

int UserInput()
{

    char option[10] = "\0", input[100] = "\0", group_name[20] = "\n", secret[20] = "\n";
    char field1[512], field2[512], auth_command[5], auth_buffer[1040], auth_rcv_buffer[1040];
    int connection_flag, n_bytes, n_pairs_kv; //n_pairs_kv = number of key-value pairs
    struct sockaddr_un other_sock_addr;
    char *token;

    //falta o bind?  pq o auth server vai ter de lhe responder com o secret
    int other_sock_addr_size = sizeof(other_sock_addr);
    strcpy(other_sock_addr.sun_path, AUTH_SOCKET_ADDR);
    other_sock_addr.sun_family = AF_INET; //alterar depois para AF_INET

    printf("Enter command (note: group names must be 100 or less characters)\n");
    if (fgets(input, 100, stdin) == NULL)
    {
        return 1;
    }

    if (!sscanf(input, " %s", option))
        return 1;

    if (strcmp(option, "create") == 0)
    {

        if (sscanf(input, " %s %s", option, group_name) == 2)
        {
            //enviar msg ao AuthServer a ver se o grupo já existe (pq não podem haver grupos iguais em computadores diferentes)
            //(basta verificar no AuthServer)

            strcpy(auth_command, "CRE_");

            assemble_payload(auth_buffer, auth_command, field1, field2, 1);
            sendto(send_socket, &auth_buffer, sizeof(auth_buffer), 0,
                   (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));

            n_bytes = recvfrom(send_socket, &connection_flag, sizeof(connection_flag), 0,
                               NULL, NULL);
            printf("A connection flag foi %d\n", connection_flag);
            cleanBuffer(auth_buffer);

            //temos que fazer strok e analisar 1º a flag
            token = strtok(auth_buffer, "_");
            connection_flag = atoi(token);

            //e se for 1 então foi sucesso e tira-se o secret
            if (connection_flag == -4)
            {
                printf("The group already exists\n");
            }
            else if (connection_flag == 1)
            {

                strcpy(secret, strtok(token, "_"));
                //se o grupo ainda não existir no AuthServer, então cria-se o grupo no LocalServer
                create_new_group(group_name);
                printf("The secret of group %s is %s\n", group_name, secret);
            }
            else
            {
                //Verificar se alguma vez entra aqui
                printf("Something went wrong creating the group in the authserver\n");
            }

            return 0;
        }
        else
            return 1;
    }
    else if (strcmp(option, "delete") == 0)
    {

        if (sscanf(input, " %s %s", option, group_name) == 2)
        {
            //verificar se o grupo existe antes de tentar apagar e se a lista não está vazia
            //verificar que o grupo faz parte deste computador, pq se existe aqui, tmb existe no AuthServer (verificação local apenas)
            hash_list *aux = groups;
            hash_list *prev = groups;

            //checks the first element of the list of groups
            if (strcmp(group_name, aux->group) == 0)
            {
                //enviar msg ao AuthServer a avisar para apagar lá o grupo e segredo
                strcpy(auth_command, "DEL_");
                assemble_payload(auth_buffer, auth_command, group_name, NULL, 1);

                /*strcpy(auth_command, "DEL_");
                sendto(send_socket, &auth_command, sizeof(auth_command), 0,
                       (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));
                sendto(send_socket, group_name, strlen(field1), 0,
                       (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));*/

                n_bytes = recvfrom(send_socket, &connection_flag, sizeof(connection_flag), 0,
                                   NULL, NULL);
                printf("A connection flag foi %d\n", connection_flag);

                if (connection_flag == -3) //deletion failed in the AuthServer
                {
                    printf("Deletion in the AuthServer failed\n");
                }
                else
                {
                    //deleting local info of the group:
                    free(aux->group_table);
                    free(aux);     //the same as free(groups);
                    groups = NULL; //the list of groups is empty now
                }
            }
            else
            {
                while (aux->next != NULL)
                {
                    if (strcmp(group_name, aux->group) == 0)
                    {
                        //enviar msg ao AuthServer a avisar para apagar lá o grupo e segredo
                        strcpy(auth_command, "DEL_");
                        //enviar apenas num sendto tudo?
                        sendto(send_socket, &auth_command, sizeof(auth_command), 0,
                               (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));
                        sendto(send_socket, group_name, strlen(field1), 0,
                               (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));

                        n_bytes = recvfrom(send_socket, &connection_flag, sizeof(connection_flag), 0,
                                           NULL, NULL);
                        printf("A connection flag foi %d\n", connection_flag);

                        if (connection_flag == -3) //deletion failed in the AuthServer
                        {
                            printf("Deletion in the AuthServer failed\n");
                        }
                        else
                        {
                            //deleting local info of the group:
                            prev->next = aux->next;
                            free(aux->group_table);
                            free(aux);
                            break;
                        }
                    }
                    prev = aux;
                    aux = aux->next;
                }
            }

            printf("That group doesn't exist\n");

            return 0;
        }
        else
            return 1;
    }
    else if (strcmp(option, "group") == 0)
    {
        hash_list *search_group;

        if (sscanf(input, " %s %s", option, group_name) == 2)
        {
            //verificar se o grupo existe
            search_group = lookup_group(group_name);
            if (search_group == NULL)
            {
                printf("The group %s doesn't exist\n", group_name);
            }
            else
            {
                //se o grupo existe, envia msg ao AuthServer a pedir o secret

                //percorrer a lista do grupo e contar o nº de elementos

                //printf("The group %s has %d key-value pairs and its secret is %s\n", group_name, n_pairs_kv, secret);
            }

            return 0;
        }
        else
            return 1;
    }
    else if (strcmp(option, "status") == 0)
    {
        //list all currently and past connected applications, with the following info: client PID, conect_time and close_time (se for o caso)
        return 0;
    }

    //if the command is invalid:
    return 1;
}

void *user_interface(void *args)
{

    while (1)
    {
        //a user interface vai precisar de receber o fd do AuthServer! (mudar depois)
        if (UserInput())
        {
            printf("Invalid command\n");
        }
    }
}

int main()
{
    int server_socket, buffer;
    struct sockaddr_un server_socket_addr, client_socket_addr;
    pthread_t t_id[1000];
    pthread_t admin;
    struct sockaddr_in socket_addr_client;
    struct sockaddr_un callback_socket_addr;

    remove(SERVER_SOCKET_ADDR);
    remove(CALLBACK_SOCKET_ADDR);

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

    callback_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (callback_socket == -1)
    {
        exit(-1);
    }

    callback_socket_addr.sun_family = AF_UNIX;
    strcpy(callback_socket_addr.sun_path, CALLBACK_SOCKET_ADDR);

    if (bind(callback_socket, (struct sockaddr *)&callback_socket_addr, sizeof(callback_socket_addr)) == -1)
    {
        printf("Callback socket already created\n");
        exit(-1);
    }

    if (listen(callback_socket, 2) == -1)
    {
        perror("listen");
        exit(-1);
    }

    send_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_socket == -1)
    {
        exit(-1);
    }
    socket_addr_client.sin_family = AF_INET;
    socket_addr_client.sin_port = htons(3000);
    socket_addr_client.sin_addr.s_addr = INADDR_ANY;

    if (bind(send_socket, (struct sockaddr *)&socket_addr_client, sizeof(socket_addr_client)) == -1)
    {
        printf("Socket from LocalServer to Auth already created\n");
        exit(-1);
    }

    //pôr em threads depois
    int i = 0;
    int client_size = sizeof(client_socket_addr);
    char buf[20];

    if (pthread_create(&admin, NULL, user_interface, NULL) != 0)
    {
        printf("Error on thread creation");
    }

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
        time_t start;
        struct tm *tm;

        time(&start);
        //clients->connection_open = localtime(&start);
        /*//if conversion is necessary, use this code:
        tm = localtime(&start);
        if (strftime(buf, sizeof(buf), "%T %D", tm) == 0)
        {
            printf("error converting start time\n");
        }
        else
        {
            printf("start time: %s %ld\n", buf, strlen(buf));
        }*/

        //guardar pid da pessoa (funcao ja esta criada)
        if ((pthread_create(&t_id[i], NULL, client_interaction, (void *)&client_fd) != 0))
        {
            printf("Error on thread creation");
        }
        i++;
    }
}