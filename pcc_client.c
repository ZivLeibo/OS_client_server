//
// Created by Ziv on 09/06/2022.
//
#define _BSD_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <endian.h>
#include <sys/stat.h>

#define BUFF_SIZE 1024
#define ERROR_MACRO {perror(""); exit(1);}

int main(int argc, char *argv[])
{
    // varivable declarations
    int sockfd = -1, filefd=-1, current_len=0;
    uint64_t data_size=0 , c=0 ,c_big_endian=0, writen_bytes = 0,result=0, data_size_big_endian=0;
    uint16_t port_num = 0;
    char *buff;
    struct sockaddr_in serv_addr;
    struct stat st;
    socklen_t addrsize;

    // input validation and open file
    if(argc !=4){perror("ERROR! wrong number of parameter!"); exit(1);}

    port_num = atoi(argv[2]);
    if (!port_num) ERROR_MACRO
    FILE *file = fopen(argv[3],"r");
    if(!file) ERROR_MACRO
    filefd = fileno(file);
    if(filefd == -1) ERROR_MACRO
    if(stat(argv[3], &st)!= 0) ERROR_MACRO
    data_size = st.st_size;

    // initilization of requested variables
    addrsize = sizeof(serv_addr);
    memset(&serv_addr, 0, addrsize);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_num); // Note: htons for endiannes
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    // create the socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0) ERROR_MACRO

    // connect socket to the target address
    if(connect(sockfd, (struct sockaddr*) &serv_addr, addrsize) < 0) ERROR_MACRO

    // write data_size to socket
    data_size_big_endian = htobe64(data_size);
    result = write(sockfd, (char*)&data_size_big_endian, sizeof(data_size_big_endian));
    if (result < sizeof (data_size_big_endian)) ERROR_MACRO

    current_len = BUFF_SIZE;

    // read stream of data from file and write it to socket
    buff = malloc(BUFF_SIZE);
    if(buff == NULL) ERROR_MACRO
    while(writen_bytes < data_size){
        if(data_size - writen_bytes < BUFF_SIZE)
            current_len = data_size - writen_bytes;

        // read from file to buf
        result = read(filefd, buff, current_len);
        if(result < current_len) ERROR_MACRO

        // write from buf to socket
        result = write(sockfd, buff, current_len);
        if (result < current_len) ERROR_MACRO
        writen_bytes += current_len;
    }

    // read data from server into buff
    result = read(sockfd, (char*)&c_big_endian, sizeof(c_big_endian));
    if(result< sizeof(c_big_endian)) ERROR_MACRO

    c = be64toh(c_big_endian);

    printf("# of printable characters: %lu\n", c);
    close(sockfd);
    close(filefd);
    exit(0);
}
