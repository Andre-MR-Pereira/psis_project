#include <stdio.h>
#ifndef _KVSLIB_H
#define _KVSLIB_H

int establish_connection(char *, char *);
int put_value(char *, char *);
int get_value(char *, char **);
int delete_value(char *);
int register_callback(char *, void (*callback_function)(char *));
int close_connection();

#endif