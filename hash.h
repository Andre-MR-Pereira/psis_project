#include <stdio.h>
#ifndef _HASH_H
#define _HASH_H

typedef struct hashtable hashtable;

struct hashtable
{
    char *key;
    char *value;
    hashtable *next;
};

unsigned hash(char *key, int size);
hashtable *lookup(hashtable **table, char *key, int size);
hashtable *insert(hashtable **table, char *key, char *value, int size);
hashtable **allocate_table();
int delete_hash(hashtable **table, char *key, int size);

#endif