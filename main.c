#include "hash.h"
#include "KVS-lib.h"
#include <stdio.h>

#define HASHSIZE 3

void *callback(char *args)
{
    printf("+++++++++++++++++++A FUNCAO DE CALLBACK FOI CHAMADA! +++++++++++++++++++\n");
    printf("%s\n", args);
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

    establish_connection("17264378291486237862318952386489236589726314879562398189234892316589216348792648791264378231849281936489213491263482163497236158728563723648792631874961238974621378964918237489234829364829731642981764789231648921364789213651723856894767489647321984628731949823164987234198236482143214231412342315342587y43758934785389653874265893746534897653829465897432589345238958932698789789786r523117234t298136478236478921634987126347982631487926314421342134123412376427381418236482713542638452317854725462315482364812634231", "17264378291486237862318952386489236589726314879562398189234892316589216348792648791264378231849281936489213491263482163497236158728563723648792631874961238974621378964918237489234829364829731642981764789231648921364789213651723856894767489647321984628731949823164987234198236482143214231412342315342587y43758934785389653874265893746534897653829465897432589345238958932698789789786r523117234t298136478236478921634987126347982631487926314421342134123412376427381418236482713542638452317854725462315482364812634231");
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