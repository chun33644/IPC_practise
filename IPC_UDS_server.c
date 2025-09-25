
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

pthread_t pid_1, pid_2;
static int end_flag = 1;

static char buff[100];

pthread_mutex_t mutex;


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



void lock(char *func_name) {
    int res = pthread_mutex_lock(&mutex);
    if (res != 0) {
        printf("func %s lock\n", func_name);
    }
}


void unlock(char *func_name) {
    int res = pthread_mutex_unlock(&mutex);
    if (res != 0) {
        printf("func %s lock\n", func_name);
    }
}



ssize_t send_task() {

    //ssize_t send(int sockfd, const void buf[size], size_t size, int flags);
    ssize_t send_bytes = send(cli_fd, buff, strlen(buff), 0);

    return send_bytes;
}


ssize_t recv_task() {

    //ssize_t recv(int sockfd, void buf[size], size_t size, int flags);
    memset(buff, 0, sizeof(buff));
    ssize_t recv_bytes = recv(cli_fd, buff, sizeof(buff), 0);
    printf("recv byte : %zd\nmsg : %s\n", recv_bytes, buff);
    return recv_bytes;
}


void* recv_and_sendBack(void *arg) {

    while (end_flag) {

        ssize_t r_bytes = recv_task();
        if (r_bytes == -1) {
            perror("Recv error");
        }

        if (buff[r_bytes] == '\0') {
            ssize_t s_bytes = send_task();
            if (s_bytes == -1) {
                perror("Send error");
                end_flag = 0;
            }
        }

    }

    return NULL;
}

/*
void* send_msg_to_client(void *arg) {

    while (sendend_flag) {

        sleep(3);

        ssize_t s_bytes = send_task();
        if (s_bytes == -1) {
            perror("Send error");
        } else if (s_bytes == 0) {
            printf("flag ?????\n");
            sendend_flag = 0;
        }

    }
    return NULL;
}
*/

int server_init() {

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
        end_flag = 0;
        close(cli_fd);
    }

    int m_res = pthread_mutex_init(&mutex, NULL);
    if (m_res != 0) {
        perror("Mutex initialization failed");
    }


    return 0;
}





int main() {

    //init
    server_init();

    int p1_res = pthread_create(&pid_1, NULL, recv_and_sendBack, NULL);
    if (p1_res != 0) {
        perror("recv_and_sendBack thread create failed");
    }
/*
    int p2_res = pthread_create(&pid_2, NULL, send_msg_to_client, NULL);
    if (p2_res != 0) {
        perror("send_handler create failed");
    }
*/

    printf("readly to join\n");

    pthread_join(pid_1, NULL);
    //pthread_join(pid_2, NULL);

    printf("join finished\n");


    close(cli_fd);


    return 0;

}
