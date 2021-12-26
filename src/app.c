#include "app.h"

connection_params params;

int check_and_initialize(char * url){
    
    if(strstr(url,"ftp://") != url) return -1;

    char * user = NULL;
    char * password = NULL;
    char * host = NULL;
    char * url_path = NULL;

    char * no_header_url = url + 6;

    char * host_component = strtok(no_header_url, "/");
    url_path = no_header_url + strlen(host_component) + 1;

    if(host_component == NULL || url_path == NULL) return -1;

    int has_auth = strstr(host_component, "@") != NULL;
 
    if(has_auth) {

        char * auth = strtok(host_component, "@");
        host = strtok(NULL, "@");

        user = strtok(auth, ":");
        password = strtok(NULL, ":");

    } else host = host_component;    

    if(valid_host(host) != 0) return -1;

    return initialize_connection_params(user, password, host, url_path);

}

int valid_host(char * host) {
    
    regex_t regex_ip, regex_hostname;
    char regex_exp_ip[] = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$";
    char regex_exp_hostname[] = "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$";

    int compiled1 = regcomp(&regex_ip, regex_exp_ip, REG_EXTENDED);
    int compiled2 = regcomp(&regex_hostname, regex_exp_hostname, REG_EXTENDED);

    if(compiled1 != 0 || compiled2 != 0) {
        printf("not compiled regex %d %d\n", compiled1, compiled2);
        return -1;
    }

    int passed_ip = regexec(&regex_ip, host, 0, NULL, 0);
    int passed_hostname = regexec(&regex_hostname, host, 0, NULL, 0);


    if(passed_ip == 0 || passed_hostname == 0) return 0;

    return -1;

}

int initialize_connection_params(const char * user, const char * password, const char * host, const char * url_path){

    if(host == NULL || url_path == NULL) return -1;

    if((user == NULL && password != NULL) ||(user != NULL && password == NULL)) return -1;

    if(user != NULL) {

        params.user = (char*) malloc(strlen(user) * sizeof(char));
        strcpy(params.user, user);
        params.password = (char*) malloc(strlen(password) * sizeof(char));
        strcpy(params.password, password);

    } else {

        params.user = NULL;
        params.password = NULL;

    }

    params.host = (char*) malloc(strlen(host) * sizeof(char));
    strcpy(params.host, host);
    params.url_path = (char*) malloc(strlen(url_path) * sizeof(char));
    strcpy(params.url_path, url_path);

    return 0;
}

void print_connection_params(){ 
    
    if(params.user != NULL) 
        printf("FTP CONNECTION - user:%s; password:%s; host:%s; url_path:%s\n", params.user, params.password, params.host, params.url_path);
    else 
        printf("FTP CONNECTION - user:anonymous; host:%s; url_path:%s\n", params.host, params.url_path);

}


