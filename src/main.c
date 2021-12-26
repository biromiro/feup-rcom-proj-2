#include "app.h"

int print_usage()
{
    printf("Usage:\tdownload ftp://[<user>:<password>@]<host>/<url-path>\n");
    return -1;
}

int main(int argc, char **argv) {

    if(argc != 2 || check_and_initialize(argv[1]) != 0) {
        print_usage();
        return -1;
    }

    return 0;
}
