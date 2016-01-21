#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>

#include <arpa/inet.h> //TODO debug - inet_ntoa()

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

//struct to hold client info & message
typedef struct client_info {

        char* message;
        struct sockaddr_in* cli;
        socklen_t cli_len;

}client_info_t;

/*******************************/
/***** Method Declarations *****/
/*******************************/
//Input Validation
int verifyPort(char*);
void initServerSocket(int*);

//Handle Message
char* messageToUpper(char*);
void readMessage(slist_t*, int);
void writeMessage(slist_t*, int);

//Misc
void freeMemory(slist_t*);

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

        int sd = 0;
        initServerSocket(&sd);

        slist_t* queue = (slist_t*)calloc(1, sizeof(slist_t));
        if(!queue) {
                perror("calloc");
                exit(EXIT_FAILURE);
        }


        fd_set readset;
        fd_set writeset;
        FD_ZERO(&readset);
        FD_ZERO(&writeset);

        while(1) {

                FD_SET(sd, &readset);
                FD_SET(sd, &writeset);

                select(sd + 1, &readset, &writeset, 0, 0);
                if(FD_ISSET(sd, &readset))
                        readMessage(queue, sd);

                if(slist_size(queue) && FD_ISSET(sd, &writeset))
                        writeMessage(queue, sd);

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

/*********************************/
/*********************************/
/*********************************/

void readMessage(slist_t* queue, int sd) {

        printf(READY_READ);
        debug_print("%s\n", "READING");

        struct sockaddr_in* cli = (struct sockaddr_in*)calloc(1, sizeof(struct sockaddr_in));
        if(!cli) {
                perror("calloc");
                freeMemory(queue);
                exit(EXIT_FAILURE);
        }
        socklen_t cli_len = sizeof(cli);

        char* message = (char*)calloc(SIZE_MESSAGE, sizeof(char));
        if(!message) {
                perror("calloc");
                free(cli);
                freeMemory(queue);
                exit(EXIT_FAILURE);
        }

        int nBytes = recvfrom(sd, message, SIZE_MESSAGE, 0, (struct sockaddr*) cli, &cli_len);

        if(nBytes < 0) {
                perror("recvfrom");
                free(cli);
                free(message);
                freeMemory(queue);
                exit(EXIT_FAILURE);
        }

        //FIXME - manually quit server. - REMOVE
        if(!strncmp(message, "quit", 4)) {
                printf("Quitting...\n");
                sendto(sd, "\n", strlen("\n"), 0, (struct sockaddr*) cli, cli_len);
                free(cli);
                free(message);
                freeMemory(queue);
                exit(EXIT_SUCCESS);
        }

        client_info_t* client_info = (client_info_t*)calloc(1, sizeof(client_info_t));
        if(!client_info) {
                perror("calloc");
                free(cli);
                free(message);
                freeMemory(queue);
                exit(EXIT_FAILURE);
        }


        messageToUpper(message);
        client_info->message = message;
        client_info->cli = cli;
        client_info->cli_len = cli_len;
        debug_print("\tmessage UPPER = %s\n\tcli mem addr = %p\n\tcli addr = %s\n",
                message,
                cli,
                inet_ntoa(cli->sin_addr));
        slist_append(queue, client_info);
}

/*********************************/
/*********************************/
/*********************************/

void writeMessage(slist_t* queue, int sd) {

        printf(READY_WRITE);
        debug_print("%s\n", "WRITING");

        client_info_t* client_info = slist_pop_first(queue);

        char* message = client_info->message;
        struct sockaddr_in* cli =  client_info->cli;
        socklen_t length = client_info->cli_len;

        debug_print("\tmessage  = %s\n\tcli mem addr = %p\n\tcli addr = %s\n",
                message,
                cli,
                inet_ntoa(cli->sin_addr));

        int nBytes = sendto(sd, message, strlen(message), 0, (struct sockaddr*) cli, length);
        if(nBytes < 0) {
                perror("sendto");
                free(message);
                free(cli);
                free(client_info);
                freeMemory(queue);
                exit(EXIT_FAILURE);
        }

        free(message);
        free(cli);
        free(client_info);
}

/******************************************************************************/
/*************************** Misc Methods *************************************/
/******************************************************************************/

void freeMemory(slist_t* queue) {

        slist_node_t* p, *q;

        for(p = slist_head(queue); p != NULL; p = q) {

                q = slist_next(p);

                client_info_t* client_info = slist_data(p);
                free(client_info->message);
                free(client_info->cli);
                free(client_info);
                free(p);
        }

        free(queue);
}
