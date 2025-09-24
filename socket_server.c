


#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>


#define SERVER_PORT             9000
#define CLIENT_MSG_BUFFSIZE     100
#define CLIENT_MAX              5


typedef struct _server_info {
    int serverfd;
    int clientsfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
} server_info;


typedef struct _client_info {
    int client_fd;
    char message[CLIENT_MSG_BUFFSIZE];
    bool connect_status;
} client_info;

typedef struct _manege_clients {
    client_info *info;
    bool in_use;
} manage_clients;

static server_info tcp;

static manage_clients info_buff[CLIENT_MAX] = {0};
static manage_clients *client_link = NULL;

static epoll_event event;


//for record maximum client
static int active_connect = 0;

//for close thread flag
static int t_flag = 1;


void signal_handler(int sig) {

    printf("%s\n", __func__);
    t_flag = 0;

}



int add_client(int new_clientfd) {

    for (int idx = 0; idx < CLIENT_MAX; idx++) {
        if (info_buff[idx].in_use) {
            continue;
        } else {
            info_buff[idx].in_use = true;
            info_buff[idx].info->client_fd = new_clientfd;
            info_buff[idx].info->connect_status = true;
            //info_buff[idx].info->message.........
            return 0;
        }
    }

    return -1;

}


// thread : check client disconnect and delete info from buff
void* client_disconnect_task(void *arg) {

    while (t_flag) {



    }


    return NULL;
}










// wait client connect and add info to buff
void* client_connect_task(void *arg) {

    while (t_flag) {

        //accept client connections
        socklen_t client_addr_size = sizeof(tcp.client_addr);
        tcp.clientsfd = accept(tcp.serverfd, (struct sockaddr*)&tcp.client_addr, &client_addr_size);
        if (tcp.clientsfd < 0) {
            perror("Accept error");
        }


        if (active_connect > CLIENT_MAX) {
            printf("Exceeded the number of connctions......\n");
            close(tcp.clientsfd);
            continue;
        } else {
            active_connect ++;
            add_client(tcp.clientsfd);
            //save client info
        }



/*
        char client_ip[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &tcp.client_addr.sin_addr, client_ip, sizeof(client_ip)) == NULL) {
            perror("inet_ntop error");
        } else {
            printf("Client connected: %s\n", client_ip);
        }
*/
    }
    return NULL;
}




// init : socket connection
int socket_init() {

    tcp.serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp.serverfd < 0) {
        perror("Socket error");
        return -1;
    }

    //set server addr struct
    memset(&tcp.server_addr, 0, sizeof(tcp.server_addr));
    tcp.server_addr.sin_family = AF_INET;
    tcp.server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //accept connections from all IP addr
    tcp.server_addr.sin_port = htons(SERVER_PORT);

    //bind socket and IP port
    if (bind(tcp.serverfd, (struct sockaddr*)&tcp.server_addr, sizeof(tcp.server_addr)) != 0) {
        perror("Bind error");
        return -2;
    }

    //monitor connection
    if (listen(tcp.serverfd, CLIENT_MAX) != 0){
        perror("Listen error");
        return -3;
    }

    return 0;
}


int main() {

    // 1.socket create (socket -> bind -> listen)
    socket_init();


    int epollfd = epoll_create1(0);
    if (epollfd < 0) {
        perror("Epoll create");
        return -1;
    }
















    // create thread for monitor connect and disconnect
    pthread_t task1;
    pthread_create(&task1, NULL, client_connect_task, NULL);


    //register a signal to close connect_thread
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    printf("If you want to close connect_thread, can use command 'kill -10 %d\n", getpid());

    printf("Waiting for all thread join....\n");
    pthread_join(task1, NULL);
    printf("All thread closed !!\n");


    close(tcp.serverfd);
    printf("Server socket closed\n");

    return 0;
}
