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

unsigned hash(char *, int);
hashtable *lookup(hashtable **, char *, int);
hashtable *insert(hashtable **, char *, char *, int);
hashtable **allocate_table();
int delete_hash(hashtable **, char *, int);

#endif