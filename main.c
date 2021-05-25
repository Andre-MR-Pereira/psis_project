#include "hash.h"
#include "KVS-lib.h"
#include <stdio.h>

#define HASHSIZE 3

void *callback(char *changed_key)
{
    printf("The key with name %s was changed\n", changed_key);
}

int main()
{
    hashtable **teste, *aux;
    teste = allocate_table(HASHSIZE);
    aux = insert(teste, "1", "Andre", HASHSIZE);
    printf("%s|%s\n", aux->key, aux->value);
    aux = insert(teste, "2", "Alex", HASHSIZE);
    printf("%s|%s\n", aux->key, aux->value);
    aux = insert(teste, "OMEGA", "MC", HASHSIZE);
    printf("%s|%s\n", aux->key, aux->value);
    aux = insert(teste, "1", "Coelho", HASHSIZE);
    printf("%s|%s\n", aux->key, aux->value);
    aux = lookup(teste, "OMEGA", HASHSIZE);
    printf("%s|%s\n", aux->key, aux->value);
    aux = insert(teste, "4", "Andre", HASHSIZE);
    printf("%s|%s\n", aux->key, aux->value);

    printf("\n---------------------------------CHECKING HASH------------------------\n");
    printf("NEW KEY 1\n");
    for (aux = teste[hash("1", HASHSIZE)]; aux != NULL; aux = aux->next)
    {
        printf("%s|%s\n", aux->key, aux->value);
    }
    printf("NEW KEY 2\n");
    for (aux = teste[hash("2", HASHSIZE)]; aux != NULL; aux = aux->next)
    {
        printf("%s|%s\n", aux->key, aux->value);
    }
    printf("NEW KEY 5\n");
    for (aux = teste[hash("5", HASHSIZE)]; aux != NULL; aux = aux->next)
    {
        printf("%s|%s\n", aux->key, aux->value);
    }
    printf("NEW KEY OMEGA\n");
    for (aux = teste[hash("OMEGA", HASHSIZE)]; aux != NULL; aux = aux->next)
    {
        printf("%s|%s\n", aux->key, aux->value);
    }

    printf("\n--DELETES--\n");
    if (delete_hash(teste, "4", HASHSIZE) == -1)
    {
        printf("Hash deletion failed\n");
    }
    if (delete_hash(teste, "1", HASHSIZE) == -1)
    {
        printf("Hash deletion failed\n");
    }
    if (delete_hash(teste, "5", HASHSIZE) == -1)
    {
        printf("Hash deletion failed\n");
    }

    printf("\n---------------------------------CHECKING HASH------------------------\n");
    printf("NEW KEY 1\n");
    for (aux = teste[hash("1", HASHSIZE)]; aux != NULL; aux = aux->next)
    {
        printf("%s|%s\n", aux->key, aux->value);
    }
    printf("NEW KEY 2\n");
    for (aux = teste[hash("2", HASHSIZE)]; aux != NULL; aux = aux->next)
    {
        printf("%s|%s\n", aux->key, aux->value);
    }
    printf("NEW KEY 5\n");
    for (aux = teste[hash("5", HASHSIZE)]; aux != NULL; aux = aux->next)
    {
        printf("%s|%s\n", aux->key, aux->value);
    }
    printf("NEW KEY OMEGA\n");
    for (aux = teste[hash("OMEGA", HASHSIZE)]; aux != NULL; aux = aux->next)
    {
        printf("%s|%s\n", aux->key, aux->value);
    }

    establish_connection("grupoteste", "1234");
    //establish_connection("grupoPSIS", "SOCKETTT");
    put_value("Laranja", "39875983759236589243659237392856723");
    put_value("Mabeco", "5");
    char *buffer;
    get_value("Laranja", &buffer);
    printf("Voltou %s\n", buffer);
    delete_value("Mabeco");
    register_callback("Laranja", (*callback)("BACANZ"));
    close_connection();
    return 0;
}