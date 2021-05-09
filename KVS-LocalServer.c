#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <pthread.h>
#include <time.h>
#include "hash.h"

#define HASHSIZE 101
#define SERVER_SOCKET_ADDR "/tmp/server_socket"

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
    struct hash_list *next;
} hash_list;

int client_fd_vector[10];
int accepted_connections = 0;
int auth_server_fd = 0;
//client_list *head_connections[10];
hash_list *groups = NULL;
client_list *clients = NULL;

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

void lookup_and_delete(char *group){

}

void *client_interaction(void *args)
{
    client_list *aux;
    hash_list *group;
    hashtable *buffer;
    int *client_buffer = (int *)args;
    int client_fd = *client_buffer;
    int pos_connects;
    int size_field1, size_field2;
    char command[5], *field1, *field2;

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
        printf("                       RECEBI COMMAND|  %s\n", command);

        switch (extract_command(command))
        {
        case 0:
            err_rcv = recv(client_fd, &size_field1, sizeof(size_field1), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            field1 = (char *)malloc((size_field1 + 1) * sizeof(char));
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
            field2 = (char *)malloc((size_field2 + 1) * sizeof(char));
            err_rcv = recv(client_fd, field2, size_field2, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            printf("EST:Recebi %s e %s\n", field1, field2);
            /*
        mandar datagram para o auth server
        */

            if (1 == 1) //verificar com o auth server
            {
                if (lookup_group(field1) == NULL)
                {
                    create_new_group(field1);
                }
                group = lookup_group(field1);
                show_groups();
                int connection_flag = 1;
                write(client_fd, &connection_flag, sizeof(connection_flag));
                free(field1);
                free(field2);
            }
            else
            {
                int error_flag = -1;
                //guardar tempo de saida
                write(client_fd, &error_flag, sizeof(error_flag));
                free(field1);
                free(field2);
                pthread_exit(NULL);
            }
            break;
        case 1:
            err_rcv = recv(client_fd, &size_field1, sizeof(size_field1), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            field1 = (char *)malloc((size_field1 + 1) * sizeof(char));
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
            field2 = (char *)malloc((size_field2 + 1) * sizeof(char));
            err_rcv = recv(client_fd, field2, size_field2, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            printf("PUT:Recebi %s e %s\n", field1, field2);
            if (group != NULL && insert(group->group_table, field1, field2, HASHSIZE) != NULL)
            {
                printf("Entra no if\n");
                int connection_flag = 1;
                write(client_fd, &connection_flag, sizeof(connection_flag));
                free(field1);
                free(field2);
            }
            else //cuidado com else else que o insert pode ser NULL
            {
                printf("You haven´t established a connection to a group yet.\n");
                int error_flag = -1;
                //guardar tempo de saida
                write(client_fd, &error_flag, sizeof(error_flag));
                free(field1);
                free(field2);
                pthread_exit(NULL);
            }
            break;
        case 2:
            err_rcv = recv(client_fd, &size_field1, sizeof(size_field1), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            field1 = (char *)malloc((size_field1 + 1) * sizeof(char));
            err_rcv = recv(client_fd, field1, size_field1, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }

            printf("GET:Recebi %s\n", field1);

            buffer = lookup(group->group_table, field1, HASHSIZE);
            printf("The value is %s\n", buffer->value);

            free(field1);

            if (1 == 1) //verificar com o auth server
            {
                int connection_flag = 1;
                write(client_fd, &connection_flag, sizeof(connection_flag));
                int size_buffer = strlen(buffer->value);
                printf("Size %d e '%s'\n", size_buffer, buffer->value);
                write(client_fd, &size_buffer, sizeof(size_buffer));
                write(client_fd, buffer->value, strlen(buffer->value));
            }
            else
            {
                int error_flag = -1;
                //guardar tempo de saida
                write(client_fd, &error_flag, sizeof(error_flag));
                pthread_exit(NULL);
            }
            break;
        case 3:
            err_rcv = recv(client_fd, &size_field1, sizeof(size_field1), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            field1 = (char *)malloc((size_field1 + 1) * sizeof(char));
            err_rcv = recv(client_fd, field1, size_field1, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }

            printf("DEL:Recebi %s\n", field1);

            if (delete_hash(group->group_table, field1, HASHSIZE) == -1)
            {
                printf("Hash deletion failed\n");
            }

            free(field1);

            if (1 == 1) //verificar com o auth server
            {
                int connection_flag = 1;
                write(client_fd, &connection_flag, sizeof(connection_flag));
            }
            else
            {
                int error_flag = -1;
                //guardar tempo de saida
                write(client_fd, &error_flag, sizeof(error_flag));
                pthread_exit(NULL);
            }
            break;
        case 4:
            err_rcv = recv(client_fd, &size_field1, sizeof(size_field1), 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            field1 = (char *)malloc((size_field1 + 1) * sizeof(char));
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
            field2 = (char *)malloc((size_field2 + 1) * sizeof(char));
            err_rcv = recv(client_fd, field2, size_field2, 0);
            if (err_rcv == -1)
            {
                perror("recieve");
                exit(-1);
            }
            printf("RCL:Recebi %s e %s\n", field1, field2);
            /*
            Associar uma callback function à key
            */
            free(field1);
            free(field2);

            if (1 == 1) //verificar com o auth server
            {
                int connection_flag = 1;
                write(client_fd, &connection_flag, sizeof(connection_flag));
            }
            else
            {
                int error_flag = -1;
                //guardar tempo de saida
                write(client_fd, &error_flag, sizeof(error_flag));
                pthread_exit(NULL);
            }
            break;
        case 5:
            printf("Closing connection with client %s", "Agora ainda nao o tenho");
            //guardar tempo de saida
            pthread_exit(NULL);
            break;
        default:
            printf("Command not found. Connection with client %d dropped\n", 1);
            int error_flag = -100;
            //guardar tempo de saida
            write(client_fd, &error_flag, sizeof(error_flag));
            pthread_exit(NULL);
        }
    }
}

int UserInput(){

	char option[10]="\0", input[100]="\0", group_name[20]="\n", secret[20]="\n";

    printf("Enter command (note: group names must be 100 or less characters)\n");
	if(fgets(input, 100, stdin)==NULL){
		return 1;
	}

	if(!sscanf(input, " %s", option)) return 1;

	if(strcmp(option, "new")==0){

        if(sscanf(input, " %s %s", option, group_name)==2 ){
            //enviar msg ao AuthServer a ver se o grupo já existe (pq não podem haver grupos iguais em computadores diferentes)
            //(basta verificar no AuthServer)

            //se o grupo ainda não existir no AuthServer, então:
            create_new_group(group_name);
            
            return 0;
        }
        else return 1;
	}
    else if(strcmp(option, "delete")==0){

        if(sscanf(input, " %s %s", option, group_name)==2){
            //verificar se o grupo existe antes de tentar apagar e se a lista não está vazia
            //verificar que o grupo faz parte deste computador, pq se existe aqui, tmb existe no AuthServer (verificação local apenas)
            hash_list *aux=groups;
            hash_list *prev=groups;

            //checks the first element of the list of groups
            if (strcmp(group_name, aux->group) == 0){
                //enviar msg ao AuthServer a avisar para apagar lá o grupo e segredo
                //PRENCHEER

                //deleting local info of the group:
                free(aux->group_table); 
                free(aux); //the same as free(groups);
                groups=NULL; //the list of groups is empty now
            }
            else{
                while(aux->next != NULL){
                    if (strcmp(group_name, aux->group) == 0){
                        //enviar msg ao AuthServer a avisar para apagar lá o grupo e segredo
                        //PRENCHEER

                        //deleting local info of the group:
                        prev->next=aux->next;
                        free(aux->group_table); 
                        free(aux);
                        break;
                    }
                    prev=aux;
                    aux=aux->next;
                }
            }

            printf("That group doesn't exist\n"); 

            return 0;
        }
        else return 1;

    }
    else if(strcmp(option, "group")==0){

        if(sscanf(input, " %s %d", option, group_name)==2){
            //verificar se o grupo existe
            

            return 0;
        }
        else return 1;

    }
    else if(strcmp(option, "status")==0){
        //list all currently and past connected applications, with the following info: client PID, conect_time and close_time (se for o caso)
        return 0;
    }  
    
    //if the command is invalid:
    return 1;
}

void *user_interface(void *args){

    while(1){
        //a user interface vai precisar de receber o fd do AuthServer! (mudar depois)
        if(UserInput()) {
            printf("Invalid command\n");
        }
    }

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

    //pôr em threads depois
    int i = 0;
    int client_size = sizeof(client_socket_addr);
    char buf[20];

    if (pthread_create(&t_id[i], NULL, user_interface, NULL) != 0){
        printf("Error on thread creation");
    }
    i++;

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
        clients->connection_open = localtime(&start);
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