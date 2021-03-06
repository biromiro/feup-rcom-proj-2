#ifndef APP_H
#define APP_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <netdb.h>

typedef struct {
    char * user;
    char * password;
    char * host;
    char * url_path;
} connection_params;

int check_and_initialize(char * url);

int valid_host(char * host);

int initialize_connection_params(const char * user, const char * password, const char * host, const char * url_path);

void print_connection_params();

int my_func();

#endif