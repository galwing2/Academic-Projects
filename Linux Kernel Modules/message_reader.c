#include <fcntl.h>     
#include <unistd.h>    
#include <sys/ioctl.h> 
#include <stdlib.h>    
#include <string.h>    
#include <errno.h>
#include <stdio.h>

#include "message_slot.h"

int main(int argc, char *argv[])
{
    int fd;
    char* file_path;
    unsigned int channel_id;
    int return_value;
    int read_ammount;
    char buffer[128]={0};

    if(argc!=3){
        perror("Illegal number of arguments");
        exit(1);
    }

    file_path = argv[1];
    channel_id = atoi(argv[2]);



    fd = open(file_path, O_RDWR);
    if (fd < 0) {
        perror("Error while opening device file");
        exit(1);
    }

    return_value = ioctl(fd, MSG_SLOT_CHANNEL, channel_id);
    if(return_value<0){
        perror("Error while assigning channel id");
        close(fd);
        exit(1);
    }

    read_ammount = read(fd,buffer,sizeof(buffer));
    if(read_ammount<0){
        perror("Error while reading message");
        close(fd);
        exit(1);
    }    
    close(fd);

    if (write(1, buffer, read_ammount) < 0) {
        perror("Error printing message");
        exit(1);
    }
    exit(0);

}