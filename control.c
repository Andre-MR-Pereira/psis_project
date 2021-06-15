#include "hash.h"
#include "KVS-lib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void callback(char *changed_key)
{
    printf("The key with name %s was changed\n", changed_key);
}

int main()
{
    char option[10] = "\0", input[520] = "\0", group_name[512] = "\n", secret[512] = "\n",value[512] = "\n",key[512] = "\n";
    while(1){
        if (fgets(input, 520, stdin) == NULL)
        {
            perror("Sentence");
            exit(-1);
        }

        if (!sscanf(input, " %s", option))
            exit(-1);

        if(strcmp(option,"establish")==0)
        {
            if (sscanf(input, " %s %s %s", option, group_name, secret) == 3)
            {
                establish_connection(group_name,secret);
            }
        }
        else if(strcmp(option,"put")==0)
        {
            if (sscanf(input, " %s %s %s", option, key,value) == 3)
            {
                put_value(key,value);
            }
        }
        else if(strcmp(option,"get")==0)
        {
            if (sscanf(input, " %s %s", option, key) == 2)
            {
                printf("get value\n");
                char *buff;
                if(get_value(key,&buff)==1)
                    printf("value %s\n",buff);
                else
                    printf("not possible\n");
            }
        }
        else if(strcmp(option,"del")==0)
        {
            if (sscanf(input, " %s %s", option, key) == 2)
            {
                delete_value(key);
            }
        }
        else if(strcmp(option,"register")==0)
        {
            if (sscanf(input, " %s %s", option, key) == 2)
            {
               register_callback(key, callback);
            }
        }
        else if(strcmp(option,"close")==0)
        {
            
            close_connection();
            
        }
        else
            printf("not a command\n");
    }
    
    return 0;
}