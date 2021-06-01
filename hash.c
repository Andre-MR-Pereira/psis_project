#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hash.h"

/* hash: form hash value for string s */
unsigned hash(char *key, int size)
{
    unsigned hashval;
    for (hashval = 0; *key != '\0'; key++)
    {
        hashval = *key + 31 * hashval;
    }
    return hashval % size;
}

/* lookup: look for s in hashtab */
hashtable *lookup(hashtable **table, char *key, int size)
{
    hashtable *aux = NULL;
    for (aux = table[hash(key, size)]; aux != NULL; aux = aux->next)
    {
        if (strcmp(key, aux->key) == 0)
            return aux; /* found */
    }
    return NULL; /* not found */
}

/* install: put (key, value) in hashtab */
hashtable *insert(hashtable **table, char *key, char *value, int size)
{
    hashtable *aux = NULL;
    unsigned hashval;
    printf("Entra %s e %s na insert\n", key, value);
    if ((aux = lookup(table, key, size)) == NULL)
    { /* not found */
        printf("Cria grupo\n");
        aux = (hashtable *)malloc(sizeof(hashtable));
        if (aux == NULL || (aux->key = strdup(key)) == NULL)
            return NULL;
        hashval = hash(key, size);
        aux->head = NULL;
        aux->next = table[hashval];
        table[hashval] = aux;
    }
    else /* already there */
        free(aux->value);
    if ((aux->value = strdup(value)) == NULL)
        return NULL;
    return aux;
}

hashtable **allocate_table(int size)
{
    hashtable **table = (hashtable **)malloc(size * sizeof(hashtable *));
    if (table == NULL)
    {
        perror("Hashtable error");
        exit(-1);
    }
    return table;
}

int delete_hash(hashtable **table, char *key, int size)
{
    int flag_head = 1;
    hashtable *aux = NULL, *prev = NULL;
    callbacks *caux = NULL, *cprev = NULL;
    unsigned hashval;
    if ((aux = lookup(table, key, size)) == NULL)
    { /* not found */
        return -1;
    }
    else
    { /* already there */
        for (aux = table[hash(key, size)]; aux != NULL; aux = aux->next)
        {
            if (strcmp(key, aux->key) == 0 && flag_head == 0)
            {
                free(aux->key);
                free(aux->value);
                caux = aux->head;
                while (caux != NULL)
                {
                    cprev = caux;
                    caux = caux->next;
                    free(cprev);
                }
                prev->next = aux->next;
                free(aux);
                break;
            }
            else if (strcmp(key, aux->key) == 0 && flag_head == 1)
            {
                prev = aux->next;
                free(aux->key);
                free(aux->value);
                caux = aux->head;
                while (caux != NULL)
                {
                    cprev = caux;
                    caux = caux->next;
                    free(cprev);
                }
                free(aux);
                table[hash(key, size)] = prev;
                break;
            }
            prev = aux;
            flag_head = 0;
        }
        return 0;
    }
}

callbacks *list_call(hashtable **table, char *key, int size)
{
    hashtable *aux = NULL;
    aux = lookup(table, key, size);
    if (aux != NULL)
    {
        return aux->head;
    }
    else
    {
        return NULL;
    }
}

int insert_callsocket(hashtable *group, int socket)
{
    callbacks *new = NULL, *caux = NULL;

    caux = group->head;
    while (caux != NULL)
    {
        if (caux->callback_socket == socket)
        {
            return -2;
        }
        caux = caux->next;
    }
    new = (callbacks *)malloc(sizeof(callbacks));
    new->callback_socket = socket;
    if (group->head == NULL)
    {
        new->next = NULL;
    }
    else
    {
        new->next = group->head;
    }
    group->head = new;
    return 0;
}

void delete_table(hashtable **table, int size)
{
    hashtable *aux = NULL, *prev = NULL;
    callbacks *caux = NULL, *cprev = NULL;
    for (int i = 0; i < size; i++)
    {
        if (table[i] != NULL)
        {
            aux = table[i];
            while (aux != NULL)
            {
                prev = aux->next;
                free(aux->key);
                free(aux->value);
                caux = aux->head;
                while (caux != NULL)
                {
                    cprev = caux;
                    caux = caux->next;
                    free(cprev);
                }
                free(aux);
                prev = aux;
            }
        }
    }
}