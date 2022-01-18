#include "app.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FTP_CTRL 21
#define MAX_LINE_SIZE 256

#define OPEN_CONNECTION 150
#define READY_USER 220
#define FILE_ACTION_SUCCESS 226
#define PASSIVE_MODE 227
#define LOGGED_IN 230
#define USER_OK_PASSWORD 331

connection_params params;

int check_and_initialize(char * url){
    
    if(strstr(url,"ftp://") != url) return -1;

    char * user = NULL;
    char * password = NULL;
    char * hostname = NULL;
    char * url_path = NULL;

    char * no_header_url = url + 6;

    size_t url_size = strlen(no_header_url);

    char * host_component = strtok(no_header_url, "/");
    if (strlen(host_component) == url_size) // no path
    {
        url_path = malloc(1);
        url_path = "";
    }
    else url_path = no_header_url + strlen(host_component) + 1;

    if(host_component == NULL || url_path == NULL) return -1;

    int has_auth = strstr(host_component, "@") != NULL;
 
    if(has_auth) {

        char * auth = strtok(host_component, "@");
        hostname = strtok(NULL, "@");

        user = strtok(auth, ":");
        password = strtok(NULL, ":");

    } else hostname = host_component;

    if(valid_host(hostname) != 0) return -1;

    return initialize_connection_params(user, password, hostname, url_path);

}

int valid_host(char * hostname) {
    
    regex_t regex_ip, regex_hostname;
    char regex_exp_ip[] = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$";
    char regex_exp_hostname[] = "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$";

    int compiled1 = regcomp(&regex_ip, regex_exp_ip, REG_EXTENDED);
    int compiled2 = regcomp(&regex_hostname, regex_exp_hostname, REG_EXTENDED);

    if(compiled1 != 0 || compiled2 != 0) {
        printf("not compiled regex %d %d\n", compiled1, compiled2);
        return -1;
    }

    int passed_ip = regexec(&regex_ip, hostname, 0, NULL, 0);
    int passed_hostname = regexec(&regex_hostname, hostname, 0, NULL, 0);


    if(passed_ip == 0 || passed_hostname == 0) return 0;

    return -1;

}

int initialize_connection_params(const char * user, const char * password, const char * hostname, const char * url_path){

    if(hostname == NULL || url_path == NULL) return -1;

    if((user == NULL && password != NULL) ||(user != NULL && password == NULL)) return -1;

    if(user != NULL) {

        params.user = (char*) malloc(strlen(user) * sizeof(char));
        strcpy(params.user, user);
        params.password = (char*) malloc(strlen(password) * sizeof(char));
        strcpy(params.password, password);

    } else {
        
        params.user = (char*) malloc(strlen("anonymous"));
        params.user = "anonymous";
        params.password = (char *) malloc(1);
        params.password = "";

    }

    struct hostent *h = gethostbyname(hostname);
    
    if (h == NULL) {
        return -1;
    }

    char* hostip = inet_ntoa(*((struct in_addr *) h->h_addr));

    params.host = (char*) malloc(strlen(hostip) * sizeof(char));
    strcpy(params.host, hostip);
    params.url_path = (char*) malloc(strlen(url_path) * sizeof(char));
    strcpy(params.url_path, url_path);

    return 0;
}

void print_connection_params(){ 
    
    if(strcmp(params.user, "anonymous") != 0)
        printf("FTP CONNECTION - user:%s; password:%s; host:%s; url_path:%s\n", params.user, params.password, params.host, params.url_path);
    else 
        printf("FTP CONNECTION - user:anonymous; host:%s; url_path:%s\n", params.host, params.url_path);

}

int open_connection(char* address, int port) {
    int fd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(fd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }
    return fd;
}

int get_answer_code(FILE* file) {
    char *line = malloc(MAX_LINE_SIZE);
    int code = -1;
    size_t n;
    
    while (getline(&line, &n, file) != -1) {
        printf("%s\n",line);
        if (line[3] == ' ') {
            code = atoi(line);
            break;
        }
    }

    free(line);

    return code;
}

int login(int fd) {
    FILE *file = fdopen(fd, "r");

    char message[MAX_LINE_SIZE];
    int ans = 0;

    while (ans != 1) {   
        ans = get_answer_code(file);

        switch (ans) {
            case READY_USER:
                sprintf(message, "USER %s\n", params.user);
                break;
            case USER_OK_PASSWORD:
                sprintf(message, "PASS %s\n", params.password);
                break;
            case LOGGED_IN:
                sprintf(message, "PASV\n");
                ans = 1;
                break;
            default:
                return -1;
        }

        printf("%s\n", message);
        write(fd, message, strlen(message));
    }

    return ans;
}

int download_file(int ctrl_fd) {

    //open connection to download

    FILE *ctrl = fdopen(ctrl_fd, "r");
    char *line = malloc(MAX_LINE_SIZE);
    size_t n;

    getline(&line, &n, ctrl);
    printf("%s\n", line);
    int code = atoi(line);
    if (code != PASSIVE_MODE) {
        free(line);
        return -1;
    }
    int address[4];
    int port[2];
    sscanf(line, "227 Entering Passive Mode (%d, %d, %d, %d, %d, %d).\n", &address[0], &address[1], &address[2], &address[3], &port[0], &port[1]);

    char address_str[16];
    sprintf(address_str, "%d.%d.%d.%d", address[0], address[1], address[2], address[3]);
    int assembled_port = 256*port[0] + port[1];

    int download_fd = open_connection(address_str, assembled_port);


    //request download

    size_t message_size = 6 + strlen(params.url_path);
    char *message = malloc(message_size);
    sprintf(message, "RETR %s\n", params.url_path);
    write(ctrl_fd, message, message_size);
    free(line);
    if (get_answer_code(ctrl) != OPEN_CONNECTION)
        return -1;


    // receive file

    char *filename = basename(params.url_path);

    int file_fd = open(filename, O_WRONLY | O_CREAT, 0666);

    char download_byte[1];

    while(read(download_fd, download_byte, 1) != 0)
        write(file_fd, download_byte, 1);

    if(get_answer_code(ctrl) != FILE_ACTION_SUCCESS)
        return -1;
    
    return 0;
}

int my_func(){

    int sockfd = open_connection(params.host, FTP_CTRL);

    int result = login(sockfd);

    if (result == 1)
        download_file(sockfd);

    if (close(sockfd)<0) {
        perror("close()");
        exit(-1);
    }

    return 0;
}
