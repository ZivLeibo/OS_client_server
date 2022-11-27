//
// Created by Ziv on 09/06/2022.
//

#define _BSD_SOURCE
#include <stdint.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <endian.h>

#define BUFF_SIZE 1024
#define DICTIONARY_LEN 95
#define DICTIONARY_PREFIX 32
#define ERROR_MACRO {perror(""); exit(1);}

// global variable
uint64_t printable_cnt[DICTIONARY_LEN], curr_printable_cnt[DICTIONARY_LEN];
uint64_t c;
int sigint_flag = 0;
struct sigaction busy, not_busy;

void print_printable_cnt(){
    char c;
    for (int i=0; i<DICTIONARY_LEN; i++){
        c = i+DICTIONARY_PREFIX;
        printf("char '%c' : %lu times\n", c, printable_cnt[i]);
    }
    exit(0);
}

void update_curr_printable_cnt(char *buf, int len){
    for (int i=0; i<len; i++)
        if(buf[i]>=32 && buf[i]<= 126) {
            c++;
            curr_printable_cnt[(int)(buf[i] - DICTIONARY_PREFIX)] += 1;
        }
}

void raise_sigint_flag() {
    sigint_flag = 1;
}

void init_signal_handlers() {
    memset(&busy, 0, sizeof(busy));
    memset(&not_busy, 0, sizeof(not_busy));

    busy.sa_handler = raise_sigint_flag;
    not_busy.sa_handler = print_printable_cnt;
}

void close_connection(int sockfd) {
    close(sockfd);
    if (sigaction(SIGINT, &not_busy, NULL) == -1) ERROR_MACRO
}

int main(int argc, char *argv[])
{
    // varivable declarations
    int listenfd  = -1, connfd = -1, current_len;
    char buff[BUFF_SIZE];
    uint64_t data_size ,data_size_big_endian, c_endian, read_bytes,result, port_num;
    struct sockaddr_in serv_addr;
    socklen_t addrsize;

    // signal handler initialization + SIGINT = not busy
    init_signal_handlers();
    if (sigaction(SIGINT, &not_busy, NULL) == -1) ERROR_MACRO

    // input validation
    if(argc != 2){perror("ERROR! wrong number of parameter!"); exit(1);}
    port_num = atoi(argv[1]);
    if(port_num==0) ERROR_MACRO

    //Initialize global pc_counters
    memset(printable_cnt, 0, sizeof(printable_cnt));
    memset(curr_printable_cnt, 0, sizeof(curr_printable_cnt));

    // create the socket, define opptions, bind
    addrsize = sizeof(struct sockaddr_in);
    memset(&serv_addr, 0, addrsize);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port_num);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd <0) ERROR_MACRO

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))!= 0) ERROR_MACRO

    if(bind(listenfd,(struct sockaddr*) &serv_addr, addrsize)!=0) ERROR_MACRO

    if(0 != listen(listenfd, 10)) ERROR_MACRO

    while(!sigint_flag)
    {
        // Accept a connection.
        connfd = accept(listenfd,NULL, NULL);
        if(connfd < 0) ERROR_MACRO

        // SIGINT hundler -> busy
        if(sigaction(SIGINT, &busy, NULL) == -1) ERROR_MACRO

        // initialize currunt cnt
        c = 0;

        // get N
        result = read(connfd, (char*)&data_size_big_endian, sizeof(data_size_big_endian));
        if (result < sizeof(data_size_big_endian)){
            perror("ERROR! read size of data packet from client failed!\n");
            close_connection(connfd);
            continue;
        }
        data_size = be64toh(data_size_big_endian);

        // get data, update data base and calc c
        current_len = BUFF_SIZE;
        read_bytes = 0;
        while(read_bytes < data_size){
            if(data_size - read_bytes < BUFF_SIZE)
                current_len = data_size - read_bytes;

            result = read(connfd, buff, current_len);

            if(result == -1){
                perror("ERROR! read data packet from client failed!\n");
                close_connection(connfd);
                continue;
            }
            read_bytes += current_len;
            update_curr_printable_cnt(buff, current_len);
        }

        // send c to client
        c_endian = htobe64(c);

        result = write(connfd, (char*)&c_endian, sizeof(c_endian));
        if(result<sizeof(c)){
            perror("ERROR! send C data packet to client failed!\n");
            close_connection(connfd);
            continue;
        }

        // close connection + set sighandler to not busy
        close_connection(connfd);

        memcpy(printable_cnt, curr_printable_cnt, sizeof(printable_cnt));
    }
    print_printable_cnt();
    exit(0);
}
