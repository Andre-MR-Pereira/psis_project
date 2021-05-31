#include "hash.h"
#include "KVS-lib.h"
#include <stdio.h>

void callback(char *changed_key)
{
    printf("The key with name %s was changed\n", changed_key);
}

int main()
{
    for (int i = 0; i < 2; i++)
    {
        establish_connection("grupoteste", "1234");
        //establish_connection("grupoPSIS", "SOCKETTT");
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
        put_value("Congresso", "500");
        delete_value("Congresso");
        delete_value("Laranja");
        close_connection();
        printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&    %d  &&&&&&&&&&&&&&&&&&&&&&&&&\n", i);
    }

    return 0;
}