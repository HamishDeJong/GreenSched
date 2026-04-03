#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/*
Accept command-line arguments
Pass them to the kps() system call
Print a usage message if no argument is provided
*/
void kpsCaller(int argc, char *argv[]){
    char buff[4];  
    char *arg;     

    if(argc < 2){
        printf("Usage: ps -o | -l\n");
        printf("Enter a kps command: ");
    

        int m = read(0, buff, sizeof(buff) - 1);
        if(m > 0){
            buff[m] = '\0';
        }

        // remove newline
        for(int i = 0; i < m; i++){
            if(buff[i] == '\n'){
                buff[i] = '\0';
                break;
            }
        }

        // if user entered nothing, exit
        if(buff[0] == '\0'){
            printf("No argument provided. Exiting.\n");
            return;
        }

        arg = buff;
    } else {
        arg = argv[1];
    }

    kps(arg);
}

int main(int argc, char *argv[]){
    printf("+++ Start lab 2 +++\n");
    kpsCaller(argc, argv);
    printf("+++ End lab 2 +++\n");
    exit(0);
}
