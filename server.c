#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include "slist.h"

#define DEBUG 1
#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

/***********************/
/***** Size Macros *****/
/***********************/
#define SIZE_MESSAGE 4096


/*****************************/
/***** Response Messages *****/
/*****************************/
#define READY_READ "Server is ready to read\n"
#define READY_WRITE "Server is ready to write\n"
#define ERROR_USAGE "Usage: ./server <port>\n"

/***********************************/
/***** Input Validation Macros *****/
/***********************************/
#define NUM_OF_EXPECTED_TOKENS 2
#define MAX_PORT 65535

/****************************/
/***** Global Variables *****/
/****************************/
int sPort = -1;

typedef struct client_info {

        char* message;
        struct sockaddr_in* cli;

}client_info_t;

/*******************************/
/***** Method Declarations *****/
/*******************************/
//Input Validation
int verifyPort(char*);
void initServerSocket(int*);

//Handle Message
char* messageToUpper(char*);


/******************************************************************************/
/******************************************************************************/
/***************************** Main Method ************************************/
/******************************************************************************/
/******************************************************************************/

int main(int argc, char* argv[]) {

        if(argc != NUM_OF_EXPECTED_TOKENS || verifyPort(argv[1])) {
                printf(ERROR_USAGE);
                exit(EXIT_FAILURE);
        }

        //TODO add frees at every program exit points;

        int sd = 0;
        initServerSocket(&sd);

        slist_t* queue = (slist_t*)calloc(1, sizeof(slist_t));
        if(!queue) {
                perror("calloc");
                exit(EXIT_FAILURE);
        }

        // char message[SIZE_MESSAGE + 1];
        // memset(message, 0, sizeof(message));

        fd_set readset;
        fd_set writeset;
        FD_ZERO(&readset);
        FD_ZERO(&writeset);

        while(1) {

                struct sockaddr_in cli;
                socklen_t cli_len = sizeof(cli);
                int nBytes = 0;

                client_info_t* client_info = (client_info_t*)calloc(1, sizeof(client_info_t));
                if(!client_info) {
                        perror("calloc");
                        exit(EXIT_FAILURE);
                }
                char* message = (char*)calloc(SIZE_MESSAGE, sizeof(char));
                if(!message) {
                        perror("calloc");
                        exit(EXIT_FAILURE);
                }

                FD_SET(sd, &readset);
                FD_SET(sd, &writeset);

                select(sd + 1, &readset, &writeset, 0, 0);
                if(FD_ISSET(sd, &readset)) {
                        printf(READY_READ);
                        nBytes = recvfrom(sd, message, SIZE_MESSAGE, 0, (struct sockaddr*) &cli, &cli_len);

                        if(nBytes < 0) {
                                perror("recvfrom");
                                exit(EXIT_FAILURE);
                        }

                        // message = messageToUpper(message);
                        messageToUpper(message);
                        client_info->message = message;
                        client_info->cli = &cli;
                        printf("message UPPER = %s\n", message);
                        printf("cli mem addr = %p\n", &cli);
                        slist_append(queue, client_info);
                }

                //any I/O operation should never block

                //maintain a queue to save read messages and the source information
                //Only when the queue in not empty you should check if the socket is ready to write
                //You always check if the socket is ready for reading operation
        }

        close(sd);
        return 0;
}


/******************************************************************************/
/******************************************************************************/
/************************ Initialize Server Methods ***************************/
/******************************************************************************/
/******************************************************************************/

int verifyPort(char* port_string) {

        //check port_string containts only digits
        int assigned = strspn(port_string, "0123456789");
        if(assigned != strlen(port_string))
                return -1;

        sPort = atoi(port_string);
        if(sPort > MAX_PORT)
                return -1;

        return 0;
}

/*********************************/
/*********************************/
/*********************************/

void initServerSocket(int* sockfd) {

        debug_print("\t%s\n", "initServerSocket");
        if((*sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
                perror("socket");
                exit(EXIT_FAILURE);
        }

        struct sockaddr_in srv;
        srv.sin_family = AF_INET;
        srv.sin_port = htons(sPort);
        srv.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(*sockfd, (struct sockaddr*) &srv, sizeof(srv)) < 0) {
                perror("bind");
                exit(EXIT_FAILURE);
        }

}

/******************************************************************************/
/******************************************************************************/
/*************************** Message Methods **********************************/
/******************************************************************************/
/******************************************************************************/

char* messageToUpper(char* message) {

        int length = strlen(message);
        int i;

        for(i = 0; i < length; i++)
                message[i] = toupper(message[i]);

        return message;
}
