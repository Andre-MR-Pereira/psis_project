#include "hash.h"
#include "KVS-lib.h"
#include <stdio.h>

void callback(char *changed_key)
{
    printf("The key with name %s was changed\n", changed_key);
}

int main()
{
    for (int i = 0; i < 5; i++)
    {
        establish_connection("grupoteste", "@5b$XkhD*R*7j3E^&E2y4r3q4^?ch?eS*U$2");
        put_value("Laranja", "39875983759236589243659237392856723");
        put_value("Mabeco", "5");
        put_value("Congresso", "200");
        char *buffer;
        get_value("Laranja", &buffer);
        printf("Voltou %s\n", buffer);
        delete_value("Mabeco");
        register_callback("Laranja", callback);
        register_callback("Congresso", callback);
        register_callback("Laranja", callback);
        printf("------------------\n");
        put_value("Congresso", "500");
        delete_value("Congresso");
        delete_value("Laranja");
        close_connection();
        printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&    %d  &&&&&&&&&&&&&&&&&&&&&&&&&\n", i);
    }
    establish_connection("grupoteste", "@5b$XkhD*R*7j3E^&E2y4r3q4^?ch?eS*U$2");
    put_value("Laranja", "39875983759236589243659237392856723");
    put_value("Mabeco", "5");
    put_value("Congresso", "200");
    put_value("Toranja", "39875983759236589243659237392858709789078907897897987129837918749721439871298479128794387219837491274987129786723");
    put_value("troco", "09");
    put_value("Bond", "007");
    put_value("Tangerina", "19");
    put_value("UBI", "AYEUSQOI9812398IUQ");
    put_value("Bean", "OQEOJNSFAN9921932189JNAJ");
    put_value("velhota", "progress");
    put_value(NULL, "5");
    put_value("parque", NULL);
    close_connection();
    return 0;
}