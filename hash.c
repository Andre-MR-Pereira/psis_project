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
    hashtable *aux;
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
    hashtable *aux;
    unsigned hashval;
    if ((aux = lookup(table, key, size)) == NULL)
    { /* not found */
        aux = (hashtable *)malloc(sizeof(hashtable));
        if (aux == NULL || (aux->key = strdup(key)) == NULL)
            return NULL;
        hashval = hash(key, size);
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
    hashtable *aux, *prev;
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
                prev->next = aux->next;
                free(aux);
                break;
            }
            else if (strcmp(key, aux->key) == 0 && flag_head == 1)
            {
                prev = aux->next;
                free(aux->key);
                free(aux->value);
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