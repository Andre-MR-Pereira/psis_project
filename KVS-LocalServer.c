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
#define AUTH_SOCKET_ADDR " 172.22.146.73" // André: "192.168.1.73"

pthread_rwlock_t groups_rwlock = PTHREAD_RWLOCK_INITIALIZER;

typedef struct client_list
{
    int pid;
    int fd;
    pthread_t t_id;
    struct sockaddr_un client_socket_addr;
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
    if (buffer[0] == '-')
    {
        flag = -(buffer[1] - '0');
    }
    else
    {
        flag = buffer[0] - '0';
    }

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

void assemble_payload(char *buffer, char *command, char *field1, char *field2)
{
    strcpy(buffer, "");
    strcat(buffer, command);
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
    if (group == NULL)
        return NULL; /* not searched for */

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

void print_group(hash_list *group)
{
    hashtable **table = group->group_table;

    printf("%s key-values:\n", group->group);

    hashtable *aux = NULL;
    if (table == NULL)
        printf("table NULL\n");
    else
    {
        for (int i = 0; i < HASHSIZE; i++)
        {
            for (aux = table[i]; aux != NULL; aux = aux->next)
            {
                //printf("Entrada %d\n", i);
                printf("%s - %s\n", aux->key, aux->value);
            }
        }
    }
    printf("\n");
}

void show_status()
{

    client_list *aux;
    printf("#### APPLICATIONS' STATUS ####\n");
    if (clients == NULL)
    {
        printf("client list is empty\n");
    }
    for (aux = clients; aux != NULL; aux = aux->next)
    {
        printf("client PID : %d | ", aux->pid);
        printf("establishing time : %ld | ", aux->connection_open);
        if (aux->connection_open == -1)
        {
            printf(" still connected\n");
        }
        else
        {
            printf("closing time : %ld \n", aux->connection_close);
        }
    }
    printf("\n");
}

void create_new_group(char *group)
{
    hash_list *aux;
    if (group == NULL)
        return;
    aux = (hash_list *)malloc(sizeof(hash_list));
    if (aux == NULL)
    {
        perror("Could not create a new group\n");
    }
    else
    {
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
}

void delete_group(char *group)
{
    //maybe useless, apagar depois
}

int count_n_elements(hash_list *group)
{
    int counter = 0;
    hashtable **table = group->group_table;
    hashtable *aux = NULL;

    if (table == NULL)
        return 0;
    else
    {
        for (int i = 0; i < HASHSIZE; i++)
        {
            for (aux = table[i]; aux != NULL; aux = aux->next)
            {
                counter++;
            }
        }
    }

    return counter;
}

char *send_with_check_response(char *buffer)
{
    //fazer aquilo dos timers aqui dentro

    //send - iniciar timer
    //check if timer expirou
    //se sim -> reenvia
    //else rcvfrom
    //retorna o buffer recebido
}

void convert_time()
{
    //just to keep this saved for now
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
}

client_list *create_new_client(int client_fd, struct sockaddr_un client)
{

    client_list *new_client = NULL;

    new_client = (client_list *)malloc(sizeof(client_list));
    if (new_client == NULL)
    {
        printf("erros allocating memory for new client\n");
    }
    else
    {
        //initialization of the new client struct
        new_client->next = NULL;
        new_client->pid = extract_pid(client);
        new_client->fd = client_fd;
        new_client->t_id = -1;
        new_client->connection_close = -1;
        //(ELEF) ver depois o que se pode inicializar logo
        //new_client->client_socket_addr=addr;

        //linkage
        if (clients == NULL) //the clients list is empty
        {
            clients = new_client;
        }
        else //insert at the head/base (ELEF) checkar se pode haver race condition
        {
            new_client->next = clients->next;
            clients->next = new_client;
        }

        //return 1; //the creation was successful
    }

    //return 0; //the creation was not successful
    return new_client;
}

void *client_interaction(void *args)
{
    client_list *aux;
    hash_list *group;
    //hash_list *aux_group;
    hashtable *buffer;
    //int *client_buffer = (int *)args;
    client_list *this_client = (client_list *)args;
    int client_fd = this_client->fd; //*client_buffer;
    int size_field1, size_field2, hooked = 0;
    int connection_flag, error_flag, n_bytes = 0;
    char command[5], field1[512], field2[512], auth_command[5], *value, auth_buffer[1040];
    struct sockaddr_in other_sock_addr;
    struct sockaddr_un client_callback_addr;
    time_t end;

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
            this_client->connection_close = time(&end);
            pthread_exit(NULL);
        }
        printf("   %d        %d|%d           RECEBI COMMAND|  %s\n", err_rcv, client_fd, this_client->pid, command);
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

            if (pthread_rwlock_rdlock(&groups_rwlock) != 0)
            {
                perror("Lock EST write lock failed");
            }
            group = lookup_group(field1);
            if (pthread_rwlock_unlock(&groups_rwlock) != 0)
            {
                perror("Unlock EST write lock failed");
            }

            if (group == NULL) //if the group doesn't exist here, the app cannot connect to it
            {
                printf("The client cannot access group: %s because it doesn't belong to this Local\n", field1);
                if (hooked == 1)
                {
                    error_flag = -4;
                }
                else
                {
                    error_flag = -2; //(ELEF) confirmar se se quer fechar a app, neste momento a flag -2 fecha
                    //guardar tempo de saida
                    this_client->connection_close = time(&end);
                }
                write(client_fd, &error_flag, sizeof(error_flag));
                pthread_exit(NULL);
            }
            else if (group->remove_flag == 1) //the group is marked deleted, so the app cannot connect
            {
                printf("Cannot establish connection. Group is flagged removed\n");
                if (hooked == 1)
                {
                    error_flag = -4;
                }
                else
                {
                    error_flag = -7; //(ELEF) confirmar se se quer fechar a app, neste momento a flag -5 não fecha
                    //guardar tempo de saida
                    //this_client->connection_close = time(&end);
                }
                write(client_fd, &error_flag, sizeof(error_flag));
                pthread_exit(NULL);
            }
            else
            {
                strcpy(auth_command, "CMP_");
                assemble_payload(auth_buffer, auth_command, field1, field2);

                n_bytes = 0;
                int i = 0;
                while (1)
                {
                    sendto(send_socket, &auth_buffer, sizeof(auth_buffer), 0,
                           (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));
                    usleep(100);
                    n_bytes = recvfrom(send_socket, &auth_buffer, sizeof(auth_buffer), MSG_DONTWAIT,
                                       NULL, NULL);
                    printf("%d  |   1 packet sent\n", i);
                    if (n_bytes > 0)
                    {
                        break;
                    }
                    if (i > 50)
                    {
                        printf("Auth server is unreachable\n");
                        strcpy(auth_buffer, "-5_");
                        break;
                    }
                    else
                    {
                        i++;
                    }
                }
                connection_flag = extract_auth(auth_buffer, field1, field2);

                if (connection_flag == 1) //verificar com o auth server
                {
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
                        this_client->connection_close = time(&end);
                    }
                    write(client_fd, &error_flag, sizeof(error_flag));
                    pthread_exit(NULL);
                }
                for (int j = 0; j < i; j++)
                {
                    n_bytes = recvfrom(send_socket, &auth_buffer, sizeof(auth_buffer), MSG_DONTWAIT,
                                       NULL, NULL);
                }
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
            value = (char *)malloc((size_field2) * sizeof(char));
            if (value == NULL)
            {
                perror("Couldn´t allocate space for value");
            }
            else
            {
                err_rcv = recv(client_fd, value, size_field2, 0);
                if (err_rcv == -1)
                {
                    perror("recieve");
                    exit(-1);
                }
                printf("PUT:Recebi %s e %s\n", field1, value);
                if (hooked == 1)
                {
                    /*int rc = pthread_rwlock_trywrlock(&group->hash_rwlock);
                    if (rc != 0)
                    {
                        printf("##########PUT nao conseguiu a WR LOCK##########\n");
                    }*/
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
            }

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

                    if (buffer->value == NULL)
                    {
                        int size_buffer = 1;
                        printf("Size %d e '%s'\n", size_buffer, "");
                        write(client_fd, &size_buffer, sizeof(size_buffer));
                        write(client_fd, "", size_buffer);
                    }
                    else
                    {
                        int size_buffer = strlen(buffer->value) + 1;
                        printf("Size %d e '%s'\n", size_buffer, buffer->value);
                        write(client_fd, &size_buffer, sizeof(size_buffer));
                        write(client_fd, buffer->value, size_buffer);
                    }
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
                    if (pthread_rwlock_unlock(&group->hash_rwlock) != 0)
                    {
                        perror("Unlock DEL write lock failed");
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
        case 5: //close_connection
            printf("Closing connection with client\n");
            hooked = 0;
            group->active_users--;
            connection_flag = 1;
            write(client_fd, &connection_flag, sizeof(connection_flag));
            //guardar tempo de saida
            this_client->connection_close = time(&end);

            pthread_exit(NULL);
            break;
        default:
            printf("Command not found. Connection with client %d dropped\n", client_fd);
            hooked = 0;
            group->active_users--;
            error_flag = -404;
            //guardar tempo de saida
            this_client->connection_close = time(&end);

            write(client_fd, &error_flag, sizeof(error_flag));
            pthread_exit(NULL);
        }
    }
}

int UserInput(struct sockaddr_in other_sock_addr)
{

    char option[10] = "\0", input[520] = "\0", group_name[512] = "\n", secret[512] = "\n";
    char field1[512], field2[512], auth_command[5], auth_buffer[1040], auth_rcv_buffer[1040];
    int connection_flag, n_bytes, n_pairs_kv, i = 0; //n_pairs_kv = number of key-value pairs
    hash_list *aux = groups;
    hash_list *prev = groups;

    printf("\nEnter command (note: group names must be 512 or less characters)\n");
    if (fgets(input, 520, stdin) == NULL)
    {
        return 1;
    }

    if (!sscanf(input, " %s", option))
        return 1;

    if (strcmp(option, "create") == 0)
    {

        if (sscanf(input, " %s %s", option, group_name) == 2)
        {
            printf("Entered create\n");
            //enviar msg ao AuthServer a ver se o grupo já existe (pq não podem haver grupos iguais em computadores diferentes)
            //(basta verificar no AuthServer)

            strcpy(auth_command, "CRE_");

            assemble_payload(auth_buffer, auth_command, group_name, NULL);

            n_bytes = 0;
            i = 0;
            while (1)
            {
                sendto(send_socket, &auth_buffer, sizeof(auth_buffer), 0,
                       (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));
                usleep(100);
                n_bytes = recvfrom(send_socket, &auth_rcv_buffer, sizeof(auth_rcv_buffer), MSG_DONTWAIT,
                                   NULL, NULL);
                printf("%d   |     1 packet sent \n", i);
                if (n_bytes > 0)
                {
                    break;
                }
                if (i > 50)
                {
                    printf("Auth server is unreachable\n");
                    strcpy(auth_rcv_buffer, "-5_");
                    break;
                }
                else
                {
                    i++;
                }
            }
            //cleanBuffer(auth_buffer);

            //temos de extrair os fields e analisar a connection flag
            printf("Recebeu %s", auth_rcv_buffer);
            connection_flag = extract_auth(auth_rcv_buffer, field1, secret);
            printf(" e a flag é %d\n", connection_flag);
            //e se for 1 então foi sucesso e tira-se o secret
            if (connection_flag == -9)
            {
                printf("The group already exists\n");
            }
            else if (connection_flag == 1)
            {
                //se o grupo ainda não existir no AuthServer, então cria-se o grupo no LocalServer
                if (pthread_rwlock_wrlock(&groups_rwlock) != 0)
                {
                    perror("Lock CRE write lock failed");
                }
                create_new_group(group_name);
                if (pthread_rwlock_unlock(&groups_rwlock) != 0)
                {
                    perror("Unlock CRE write lock failed");
                }
                printf("The secret of group %s is %s\n", group_name, secret);
            }
            else
            {
                //Verificar se alguma vez entra aqui
                printf("Something went wrong creating the group in the authserver\n");
            }
            //receives the answers of resent messages -> not good if the packages are actually lost
            for (int j = 0; j < i; j++)
            {
                n_bytes = recvfrom(send_socket, &auth_rcv_buffer, sizeof(auth_rcv_buffer), MSG_DONTWAIT,
                                   NULL, NULL);
            }
            return 0;
        }
        else
            return 1;
    }
    else if (strcmp(option, "delete") == 0)
    {
        printf("Entered delete\n");
        if (sscanf(input, " %s %s", option, group_name) == 2)
        {
            //verificar se o grupo existe antes de tentar apagar e se a lista não está vazia
            //verificar que o grupo faz parte deste computador, pq se existe aqui, tmb existe no AuthServer (verificação local apenas)

            //read lock nos grupos?? (André?)
            if (pthread_rwlock_rdlock(&groups_rwlock) != 0)
            {
                perror("Lock DEL read lock failed");
            }
            //checks the first element of the list of groups
            if (strcmp(group_name, aux->group) == 0)
            {
                //the group was found, unlocks the read lock
                if (pthread_rwlock_unlock(&groups_rwlock) != 0)
                {
                    perror("Unlock DEL read lock failed");
                }

                aux->remove_flag = 1;

                //enviar msg ao AuthServer a avisar para apagar lá o grupo e segredo
                strcpy(auth_command, "DEL_");
                assemble_payload(auth_buffer, auth_command, group_name, NULL);

                n_bytes = 0;
                i = 0;
                while (1)
                {
                    sendto(send_socket, &auth_buffer, sizeof(auth_buffer), 0,
                           (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));
                    usleep(100);
                    n_bytes = recvfrom(send_socket, &auth_rcv_buffer, sizeof(auth_rcv_buffer), MSG_DONTWAIT,
                                       NULL, NULL);
                    printf("%d  |  1 packet sent\n", i);
                    if (n_bytes > 0)
                    {
                        break;
                    }
                    if (i > 50)
                    {
                        printf("Auth server is unreachable\n");
                        strcpy(auth_rcv_buffer, "-3_");
                        break;
                    }
                    else
                    {
                        i++;
                    }
                }

                //temos de extrair os fields e analisar a connection flag
                connection_flag = extract_auth(auth_rcv_buffer, field1, secret);
                printf("A connection flag foi %d\n", connection_flag);

                if (connection_flag == 1) //deletion was successful in the AuthServer
                {
                    if (aux->active_users == 0) //if no application is using this group, then it's deleted
                    {
                        //deleting local info of the group:
                        //since the group is right away the first, it's only necessary to change the first pointer
                        if (pthread_rwlock_wrlock(&groups_rwlock) != 0)
                        {
                            perror("Lock DEL write lock failed");
                        }
                        groups = aux->next;
                        if (pthread_rwlock_unlock(&groups_rwlock) != 0)
                        {
                            perror("Unlock DEL write lock failed");
                        }
                        delete_table(aux->group_table, HASHSIZE);
                        free(aux); //the same as free(groups);
                    }
                    else
                    {
                        printf("The group is still in use. It can't be deleted\n");
                    }
                }
                else if (connection_flag == -3) //deletion failed in the AuthServer
                {
                    printf("Deletion in the AuthServer failed\n");
                }
                else
                {
                    printf("Deletion unknown flag\n");
                }
                for (int j = 0; j < i; j++)
                {
                    n_bytes = recvfrom(send_socket, &auth_rcv_buffer, sizeof(auth_rcv_buffer), MSG_DONTWAIT,
                                       NULL, NULL);
                }
                return 0;
            }
            else //searches the rest of the list
            {
                prev = aux;
                aux = aux->next;
                while (aux != NULL)
                {
                    if (strcmp(group_name, aux->group) == 0)
                    {
                        //the group was found, unlocks the read lock
                        if (pthread_rwlock_unlock(&groups_rwlock) != 0)
                        {
                            perror("Unlock DEL read lock failed");
                        }

                        aux->remove_flag = 1;

                        //enviar msg ao AuthServer a avisar para apagar lá o grupo e segredo
                        strcpy(auth_command, "DEL_");
                        assemble_payload(auth_buffer, auth_command, group_name, NULL);

                        n_bytes = 0;
                        i = 0;
                        while (1)
                        {
                            sendto(send_socket, &auth_buffer, sizeof(auth_buffer), 0,
                                   (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));
                            usleep(100);
                            n_bytes = recvfrom(send_socket, &auth_rcv_buffer, sizeof(auth_rcv_buffer), MSG_DONTWAIT,
                                               NULL, NULL);
                            printf("%d  |  1 packet sent\n", i);
                            if (n_bytes > 0)
                            {
                                break;
                            }
                            if (i > 50)
                            {
                                printf("Auth server is unreachable\n");
                                strcpy(auth_rcv_buffer, "-3_");
                                break;
                            }
                            else
                            {
                                i++;
                            }
                        }

                        //temos de extrair os fields e analisar a connection flag
                        connection_flag = extract_auth(auth_rcv_buffer, field1, secret);
                        printf("A connection flag foi %d\n", connection_flag);

                        if (connection_flag == 1) //deletion failed in the AuthServer
                        {
                            if (aux->active_users == 0) //if no application is using this group, then it's deleted
                            {
                                //deleting local info of the group:
                                if (pthread_rwlock_wrlock(&groups_rwlock) != 0)
                                {
                                    perror("Lock DEL write lock failed");
                                }
                                prev->next = aux->next;
                                if (pthread_rwlock_unlock(&groups_rwlock) != 0)
                                {
                                    perror("Unlock DEL write lock failed");
                                }
                                delete_table(aux->group_table, HASHSIZE);
                                free(aux); //the same as free(groups);
                            }
                            else
                            {
                                printf("The group is still in use. It can't be deleted\n");
                            }
                        }
                        else if (connection_flag == -3) //deletion failed in the AuthServer
                        {
                            printf("Deletion in the AuthServer failed\n");
                        }
                        else
                        {
                            printf("Deletion unknown flag\n");
                        }
                        for (int j = 0; j < i; j++)
                        {
                            n_bytes = recvfrom(send_socket, &auth_rcv_buffer, sizeof(auth_rcv_buffer), MSG_DONTWAIT,
                                               NULL, NULL);
                        }
                        return 0;
                    }
                    prev = aux;
                    aux = aux->next;
                }
                return 1;
            }
        }
    }
    else if (strcmp(option, "group") == 0)
    {
        printf("Entered group\n");
        hash_list *search_group;

        if (sscanf(input, " %s %s", option, group_name) == 2)
        {
            //verificar se o grupo existe
            if (pthread_rwlock_rdlock(&groups_rwlock) != 0)
            {
                perror("Lock GROUP read lock failed");
            }
            search_group = lookup_group(group_name);
            if (pthread_rwlock_unlock(&groups_rwlock) != 0)
            {
                perror("Lock GROUP read unlock failed");
            }

            if (search_group == NULL)
            {
                printf("The group %s doesn't exist\n", group_name);
            }
            else
            {
                //se o grupo existe, envia msg ao AuthServer a pedir o secret
                strcpy(auth_command, "ASK_");
                assemble_payload(auth_buffer, auth_command, group_name, NULL);

                n_bytes = 0;
                i = 0;
                while (1)
                {
                    sendto(send_socket, &auth_buffer, sizeof(auth_buffer), 0,
                           (struct sockaddr *)&other_sock_addr, sizeof(other_sock_addr));
                    usleep(100);
                    n_bytes = recvfrom(send_socket, &auth_rcv_buffer, sizeof(auth_rcv_buffer), MSG_DONTWAIT,
                                       NULL, NULL);
                    printf("%d   |    1 packet sent\n", i);
                    if (n_bytes > 0)
                    {
                        break;
                    }
                    if (i > 50)
                    {
                        printf("Auth server is unreachable\n");
                        strcpy(auth_rcv_buffer, "-5_");
                        break;
                    }
                    else
                    {
                        i++;
                    }
                }

                //temos de extrair os fields e analisar a connection flag
                connection_flag = extract_auth(auth_rcv_buffer, field1, secret);
                printf("A connection flag foi %d\n", connection_flag);

                if (connection_flag == 1)
                {
                    if (pthread_rwlock_rdlock(&search_group->hash_rwlock) != 0)
                    {
                        perror("Lock GROUP_count read lock failed");
                    }
                    //percorrer a lista do grupo e contar o nº de elementos
                    n_pairs_kv = count_n_elements(search_group);
                    if (pthread_rwlock_unlock(&search_group->hash_rwlock) != 0)
                    {
                        perror("Lock GROUP_count read unlock failed");
                    }

                    printf("The group %s has %d key-value pairs and its secret is %s\n", group_name, n_pairs_kv, secret);
                }
                else if (connection_flag == -2)
                {
                    perror("group doesn't exist in auth");
                }
                else
                {
                    //default
                    printf("Auth server malfunction - ASK\n");
                }
                //receives the answers of resent messages
                for (int j = 0; j < i; j++)
                {
                    n_bytes = recvfrom(send_socket, &auth_rcv_buffer, sizeof(auth_rcv_buffer), MSG_DONTWAIT,
                                    NULL, NULL);
                }
            }

            return 0;
        }
        else
            return 1;
    }
    else if (strcmp(option, "status") == 0)
    {
        printf("Entered status\n");
        //list all currently and past connected applications, with the following info:
        //client PID, connect_time and close_time (se for o caso)
        show_status();
        return 0;
    }
    else if (strcmp(option, "show") == 0)
    {
        printf("Entered show group\n");
        if (sscanf(input, " %s %s", option, group_name) == 2)
        {
            aux = lookup_group(group_name);

            if (aux != NULL)
            {
                print_group(aux);
            }
            else
            {
                printf("Group cannot be printed");
            }
        }
        return 0;
    }
    //if the command is invalid:
    return 1;
}

void *user_interface(void *args)
{
    struct sockaddr_in other_sock_addr;

    other_sock_addr.sin_family = AF_INET;
    inet_aton(AUTH_SOCKET_ADDR, &other_sock_addr.sin_addr);
    other_sock_addr.sin_port = htons(3001);

    while (1)
    {
        //a user interface vai precisar de receber o fd do AuthServer! (mudar depois)
        if (UserInput(other_sock_addr))
        {
            printf("Invalid command\n");
        }
    }
}

int main()
{
    int server_socket, buffer;
    client_list *this_client;
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
    //int i = 0;
    int client_size = sizeof(client_socket_addr);
    //char buf[20];
    time_t start;
    struct tm *tm; //for the conversion, maybe not needed

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

        //client_fd_vector[i] = client_fd;
        //accepted_connections++;

        if ((this_client = create_new_client(client_fd, client_socket_addr)) != NULL) //==1)
        {
            //(ELEF) take care of possible race condition! criar uma rwlock para os clients

            //guardar tempo conexão
            this_client->connection_open = time(&start);

            //create new thread for the client
            if ((pthread_create(&(this_client->t_id), NULL, client_interaction, (void *)this_client))) //(void *)&client_fd) != 0))
            {
                printf("Error on thread creation");
            }
        }

        /*time_t start;
        struct tm *tm;
        clients->connection_open = localtime(&start);
        //if conversion is necessary, use this code:
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
        /*if ((pthread_create(&t_id[i], NULL, client_interaction, (void *)&client_fd) != 0))
        {
            printf("Error on thread creation");
        }
        i++;*/
    }
}