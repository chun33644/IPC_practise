

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9000

int sock;  //for test thread use


void *thread_send_data(void *arg) {

    char msg[50];
    int thread_id = *((int *)arg);
    free(arg);

    for (int idx = 0; idx < 5; idx ++) {
        sprintf(msg, "message from thread : %d, count : %d\n", thread_id, idx);
        write(sock, msg, strlen(msg));
        usleep(100000);  //delay
    }

    return NULL;
}



int main () {
    //int sock;
    struct sockaddr_in server_addr;
/*
    char message[] ="Hello, server!";
    char buffer[1024];
*/

    //create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket error");
        return -1;
    }
    printf("Client socket %d\n", sock);

    //init
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    //connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect error");
        return -2;
    }


    /*----for test----*/
    printf("connected to server. starting threads to send data.....\n");

    pthread_t t1, t2;
    int *id1 = malloc(sizeof(int));
    *id1 = 1;
    int *id2 = malloc(sizeof(int));
    *id2 = 2;

    pthread_create(&t1, NULL, thread_send_data, (void *)id1);
    pthread_create(&t2, NULL, thread_send_data, (void *)id2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("join finished\n");


    /*----------------*/


    /*
    //send msg
    write(sock, message, sizeof(message));

    //received msg
    read(sock, buffer, sizeof(buffer));
    printf("Received from server: %s\n", buffer);
    */

    //close connections
    close(sock);
    printf("sock closed\n");

    return 0;
}
