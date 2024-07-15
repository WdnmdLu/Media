#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
void Process(int fd,const char *filename);

int main(int argc,char *argv[]){
    printf("Hi, this is Flv parser test\n");
    if(argc != 3){
        printf("FlvParser.exe [input flv] [output flv]\n");
        return 0;
    }
    int fd = open(argv[1],O_WRONLY);
    if(fd < 0){
        perror("Open(): ");
        return 0;
    }
    Process(fd,argv[2]);
    close(fd);
    return 1;
}

void Process(int fd,const char *filename){
    
}