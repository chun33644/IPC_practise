
#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>

#define  SERVER_PATH       "UDS_Server_socket"

int cli_fd;
socklen_t len;
struct sockaddr_un addrs;

/*
 *  struct sockaddr_un {
 *      sa_family_t     sun_family;
 *      char            sun_path[];
 *  };
*/


pthread_t pid_1;
pthread_t pid_2;

static char buff[100];

ssize_t recv_task() {

    //ssize_t recv(int sockfd, void buf[size], size_t size, int flags);
    memset(&buff, 0, sizeof(buff));
    ssize_t recv_bytes = recv(cli_fd, buff, sizeof(buff), 0);
    printf("%s\n", buff);
    return recv_bytes;
}



ssize_t send_task() {

    //ssize_t send(int sockfd, const void buf[size], size_t size, int flags);
    char *msg = "[send]this msg from client send.";
    ssize_t send_bytes = send(cli_fd, msg, strlen(msg), 0);
    return send_bytes;
}




void* send_msg_to_server(void *arg) {

    while (1) {
        send_task();
        sleep(3);

    }
   return NULL;
}


void* recv_msg_from_server(void *arg) {

    while(1) {
        recv_task();
    }
    return NULL;
}



int client_sock_init() {

    cli_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cli_fd == -1) {
        perror("Socket error");
    }


    addrs.sun_family = AF_UNIX;
    strcpy(addrs.sun_path, SERVER_PATH);
    len = sizeof(addrs);

    int result = connect(cli_fd, (struct sockaddr *)&addrs, len);
    if (result == -1) {
        perror("Connect error");
    }
    return 0;

}


int main() {


    //UDS init
    client_sock_init();

    int p1_res = pthread_create(&pid_1, NULL, send_msg_to_server, NULL);
    if (p1_res != 0) {
        perror("thread 1 create fail");
    }

    int p2_res = pthread_create(&pid_2, NULL, recv_msg_from_server, NULL);
    if (p2_res != 0) {
        perror("thread 2 create fail");
    }

    printf("readly to join\n");

    pthread_join(pid_2, NULL);
    printf("[pid_2] join finished\n");

    pthread_join(pid_1, NULL);
    printf("[pid_1] join finished\n");



/*
    ssize_t s_bytes = send_task();
    if (s_bytes < 0) {
        perror("Send error");
    }

    ssize_t r_bytes = recv_task();
    if (r_bytes == -1) {
        perror("Recv error");
    }
*/

    close(cli_fd);
    return 0;

}
