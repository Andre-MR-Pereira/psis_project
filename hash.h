#include <stdio.h>
#ifndef _HASH_H
#define _HASH_H

typedef struct hashtable hashtable;
typedef struct callbacks callbacks;

struct callbacks
{
    int callback_socket;
    callbacks *next;
};

struct hashtable
{
    char *key;
    char *value;
    callbacks *head;
    hashtable *next;
};

unsigned hash(char *key, int size);
hashtable *lookup(hashtable **table, char *key, int size);
hashtable *insert(hashtable **table, char *key, char *value, int size);
hashtable **allocate_table();
int delete_hash(hashtable **table, char *key, int size);
callbacks *list_call(hashtable **table, char *key, int size);
int insert_callsocket(hashtable *group, int socket);
void delete_table(hashtable **table, int size);

#endif