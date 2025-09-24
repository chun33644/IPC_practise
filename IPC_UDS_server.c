
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>

#define  SERVER_PATH       "UDS_Server_socket"

int ser_fd, cli_fd;
socklen_t ser_len, cli_len;
struct sockaddr_un ser_addr, cli_addr;

/*
 *  struct sockaddr_un {
 *      sa_family_t     sun_family;
 *      char            sun_path[];
 *  };
*/


typedef struct _client_info {
    int cfd;
    char mas_buff[1024];
} client_info;

pthread_t pid_1;
static int loop_flag = 1;


struct sockaddr_storage storage;
socklen_t storage_len = sizeof(struct sockaddr_storage);


/* result: unable to obtain sun_path.....*/
void print_client_info() {

    struct sockaddr_un *client = (struct sockaddr_un *)&storage;
    //int getsockname(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
    int res = getsockname(ser_fd, (struct sockaddr *)&client, &storage_len);
    if (res == -1) {
        perror("Getsockname error");
    }
    printf("client path: %s\n", client->sun_path);

}


ssize_t write_task() {

    //ssize_t write(int fd, const void buf[count], size_t count);
    char *msg = "[write]this msg from server send.";
    ssize_t write_bytes = write(cli_fd, msg, strlen(msg));

    return write_bytes;
}



ssize_t send_task() {

    //ssize_t send(int sockfd, const void buf[size], size_t size, int flags);
    char *msg = "[send]this msg from server send.";
    ssize_t send_bytes = send(cli_fd, msg, strlen(msg), 0);

    return send_bytes;
}


ssize_t read_task() {

    //ssize_t read(int fd, void buf[count], size_t count);
    char buff[100];
    ssize_t read_bytes = read(cli_fd, buff, sizeof(buff));
    printf("recv byte : %zd\nmsg : %s\n", read_bytes, buff);

    return read_bytes;
}




ssize_t recv_task() {

    //ssize_t recv(int sockfd, void buf[size], size_t size, int flags);
    char buff[100];
    ssize_t recv_bytes = recv(cli_fd, buff, sizeof(buff), 0);
    printf("recv byte : %zd\nmsg : %s\n", recv_bytes, buff);
    memset(&buff, 0, sizeof(buff));

    return recv_bytes;
}



void* ser_p1_handler(void *arg) {

    while (loop_flag) {

        ssize_t r_bytes = recv_task();
        if (r_bytes == -1) {
            perror("Recv error");
        } else if (r_bytes == 0) {
            loop_flag = 0;
        }

    }

    return NULL;
}


int server_sock_init() {

    ser_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ser_fd == -1) {
        perror("Socket error");
    }


    memset(&ser_addr, 0, sizeof(ser_addr));
    unlink(SERVER_PATH);
    ser_addr.sun_family = AF_UNIX;
    strcpy(ser_addr.sun_path, SERVER_PATH);
    ser_len = sizeof(ser_addr);

    int bind_res = bind(ser_fd, (struct sockaddr *)&ser_addr, ser_len);
    if (bind_res == -1) {
        perror("Bind error");
    }

    printf("server path: %s\n", ser_addr.sun_path);

    int listen_res = listen(ser_fd, 5);
    if (listen_res == -1) {
        perror("Listen error");
    }


    cli_len = sizeof(cli_addr);
    cli_fd = accept(ser_fd, (struct sockaddr *)&cli_addr, &(cli_len));
    if (cli_fd == -1) {
        perror("Accept error");
        loop_flag = 0;
    }


    return 0;
}


int main() {

    //UDS init
    server_sock_init();

    //thread_1 : loop (recv)
    int p1_res = pthread_create(&pid_1, NULL, ser_p1_handler, NULL);
    if (p1_res != 0) {
        perror("thread 1 create fail");
    }

    printf("readly to join\n");
    pthread_join(pid_1, NULL);
    printf("join finished\n");

/*
    ssize_t s_bytes = send_task();
    if (s_bytes < 0) {
        perror("Send error");
    }
*/


    close(cli_fd);


    return 0;

}
