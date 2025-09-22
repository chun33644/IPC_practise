


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define SERVER_PORT    9000

int main() {

    //serverfd > use to monitor new connections
    //clientfd > use to exchange msg with a specific client
    int serverfd, clientfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    char message[] = "Hello, client! (is msg from sever send)\n";
    char buffer[1024];


    //create socketi
    //int socket(int domain, int type, int protocol);
    //retuen > 0 get a file descriptor
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0) {
        perror("socket error");
        return -1;
    }
    printf("Server socket %d\n", serverfd);

    //set server addr struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //accept connections from all IP addr
    server_addr.sin_port = htons(SERVER_PORT);


    //bind socket and IP port
    //int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    if (bind(serverfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        perror("bind error");
        return -2;
    }


    //monitor connection, setting maximum 5 of connections
    //int listen(int sockfd, int backlog);
    if (listen(serverfd, 5) != 0){
        perror("listen error");
        return -3;
    }

    //accept client connections
    client_addr_size = sizeof(client_addr);
    clientfd = accept(serverfd, (struct sockaddr*)&client_addr, &client_addr_size);
    if (clientfd < 0) {
        perror("accept error");
        return -4;
    }
    printf("Accept socket %d\n", clientfd);

    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip)) == NULL) {
        perror("inet_ntop error");
        return -5;
    } else {
        printf("Client connected: %s\n", client_ip);
    }


    /*--------for test--------*/
    ssize_t read_bytes = read(clientfd, buffer, sizeof(buffer));
    while(1) {
        printf("received from client: %s\n", buffer);
        // check client thread write.....
    }
    printf("client disconnectd.\n");



/*
    //received and send msg
    read(clientfd, buffer, sizeof(buffer));
    printf("%s\n", buffer);
    write(clientfd, message, sizeof(message));
*/


    //close connections
    close(clientfd);
    close(serverfd);

    return 0;
}
