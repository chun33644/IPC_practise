
#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "IPC_UDS_common.h"

#define  SERVER_PATH       "UDS_Server_socket"
#define  CLI_MAX           3

static client_info list[CLI_MAX] = {0};

static pthread_mutex_t mutex;

static pthread_t pid_1;

static char buff[100];


void* send_msg_to_server(void *arg) {


    while (1) {

        client_info *ptr = (client_info *)arg;

        snprintf(ptr->pkg.msg, 100, "this msg from client %d send.", ptr->client.fd);

        if (!ptr->start_flag) {
            break;
        } else if (ptr->start_flag) {
            ssize_t s_res = send(ptr->client.fd, &ptr->pkg, sizeof(package), 0);
            if (s_res == -1) {
                perror("Send error");
                return NULL;
            }

            memset(ptr->pkg.msg, 0, sizeof(package));
            sleep(2);
            // printf("ready to close .... fd %d", ptr->client.fd);
            //error_handler(ptr);
        }
    }
    return NULL;
}


void* recv_msg_from_server(void *arg) {

    while(1) {

        client_info *ptr = (client_info *)arg;
        if (!ptr->start_flag) {
            break;
        } else if (ptr->start_flag) {

            ssize_t r_res = recv(ptr->client.fd, &ptr->pkg, sizeof(package), 0);
            if (r_res < 0) {
                perror("Recv error");
                continue;
            }

            printf("%s\n", ptr->pkg.msg);
        }
    }
    return NULL;
}


int create_client_UDS(int *cl_fd, struct sockaddr_un *cl_addr, socklen_t *cl_len) {

    *cl_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (*cl_fd == -1) {
        perror("Socket error");
        return -1;
    }

    cl_addr->sun_family = AF_UNIX;
    strcpy(cl_addr->sun_path, SERVER_PATH);
    *cl_len= sizeof(struct sockaddr_un);

    int result = connect(*cl_fd, (struct sockaddr *)cl_addr, *cl_len);
    if (result == -1) {
        perror("Connect error");
        close(*cl_fd);
        return -2;
    }
    return 0;
}


void* create_multiple_clientfd(void *arg) {

    client_info *ptr = (client_info *)arg;
    printf("for test 1.....\n");
    for (int idx = 0; idx < CLI_MAX; idx ++) {
        int res = create_client_UDS(&(ptr[idx].client.fd), &(ptr[idx].client.addr), &(ptr[idx].client.len));
        if (res < 0) {
            printf("idx[%d]create client failed.\n", idx);
            return NULL;
        }
        printf("[%d]fd:%d\n", idx, ptr[idx].client.fd);

        ptr[idx].start_flag = 1;
        int s_res = pthread_create(&(ptr[idx].s_pid), NULL, send_msg_to_server, (client_info *)&ptr[idx]);
        int r_res = pthread_create(&(ptr[idx].r_pid), NULL, recv_msg_from_server, (client_info *)&ptr[idx]);
        if (s_res != 0 || r_res != 0) {
            printf("list[%d] recv or send thread create failed....\n", idx);
            error_handler(&ptr[idx]);
        }
        ptr[idx].in_use = true;
    }

    for (int idx = 0; idx < CLI_MAX; idx ++) {
        pthread_join(ptr[idx].s_pid, NULL);
        pthread_join(ptr[idx].r_pid, NULL);
    }

    return NULL;
}

int main() {

    //Mutex init
    int m_res = pthread_mutex_init(&mutex, NULL);
    if (m_res != 0) {
        perror("Mutex initialization failed");
    }

    //Thread_1
    int p1_res = pthread_create(&pid_1, NULL, create_multiple_clientfd, (client_info *)list);
    if (p1_res != 0) {
        perror("Thread 2 create failed");
    }

    pthread_join(pid_1, NULL);

    //close

    return 0;

}
