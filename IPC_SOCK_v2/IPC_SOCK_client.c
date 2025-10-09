
#include "IPC_SOCK_config.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "IPC_SOCK_general.h"

static client_info list[CLI_MAX] = {0};

static pthread_mutex_t mutex;

static pthread_t pid;


int send_task(client_info *ptr) {

    while (flag) {

        char msg[MSG_MAX] = {0};
        snprintf(msg, MSG_MAX, "this msg from client %d send.", ptr->s_info.fd);

        ssize_t s_res = send(ptr->s_info.fd, msg, sizeof(msg), 0);
        if (s_res == -1) {
            perror("Send error");
            error_handler(ptr);
            return -1;
        }
        printf("Send msg:%s\n", msg);
        memset(msg, 0, sizeof(msg));

    }
    return 0;
}


void* create_multiple_clientfd(void *arg) {

    client_info *ptr = (client_info *)arg;

    for (int idx = 0; idx < CLI_MAX; idx ++) {
        int res = client_connect_init(&ptr[idx].s_info);
        if (res < 0) {
            printf("idx[%d]create s_info failed.\n", idx);
            return NULL;
        }
        printf("[%d]fd:%d\n", idx, ptr[idx].s_info.fd);
        ptr[idx].in_use = true;

        send_task(&ptr[idx]);

    }
    return NULL;
}



int main() {

    //Mutex init
    int m_res = pthread_mutex_init(&mutex, NULL);
    if (m_res != 0) {
        perror("Mutex initialization failed");
    }

    //Thread
    int p1_res = pthread_create(&pid, NULL, create_multiple_clientfd, (client_info *)list);
    if (p1_res != 0) {
        perror("Thread 2 create failed");
    }

    pthread_join(pid, NULL);

    for (int idx = 0; idx < CLI_MAX; idx ++) {
        close(list[idx].s_info.fd);
    }

    return 0;

}
