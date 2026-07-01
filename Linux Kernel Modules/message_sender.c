#include <fcntl.h>      /* for open and O_RDWR */ 
#include <unistd.h>     /* for write and close */
#include <sys/ioctl.h>  /* for ioctl */
#include <stdlib.h>     /* for atoi */
#include <string.h>     /* for strlen */
#include <errno.h>
#include <stdio.h>

#include "message_slot.h"

int main(int argc, char *argv[])
{
    int fd;
    char* file_path;
    char* message;
    unsigned int channel_id;
    unsigned int censorship_mode;
    int return_value;

    if(argc!=5){
        perror("Illegal number of arguments");
        exit(1);
    }

    file_path = argv[1];
    channel_id = atoi(argv[2]);
    censorship_mode = atoi(argv[3]);
    message = argv[4];

    fd = open(file_path, O_RDWR);
    if (fd < 0) {
        perror("Error while opening device file");
        exit(1);
    }
    return_value = ioctl(fd, MSG_SLOT_SET_CEN, censorship_mode);
    if(return_value<0){
        perror("Error while setting censorship mode");
        close(fd);
        exit(1);
    }

    return_value = ioctl(fd, MSG_SLOT_CHANNEL, channel_id);
    if(return_value<0){
        perror("Error while assigning channel id");
        close(fd);
        exit(1);
    }

    return_value = write(fd, message, strlen(message));
    if(return_value<0){
        perror("Error while writing message");
        close(fd);
        exit(1);
    }
    close(fd);
    exit(0);


    return EXIT_SUCCESS;
}