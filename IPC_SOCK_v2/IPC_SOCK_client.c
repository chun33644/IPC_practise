
#include "IPC_SOCK_config.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#include "IPC_SOCK_general.h"

static client_info list[CLI_MAX] = {0};

static pthread_mutex_t mutex;

static pthread_t pid;




void revc_notify_task(mqd_t mq_d, package *pkg) {

    ssize_t bytes = mq_receive(mq_d, (char *)pkg, sizeof(*pkg), 0);
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        perror("mq_receive");
        mq_close(mq_d);
        return;
    }
    printf("%s(len%d)\n", pkg->msg, pkg->payload_len);

}


int send_task(client_info *ptr) {

    while(1) {

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


void confrim_connect(mqd_t mq_d, package *pkg){

    ssize_t bytes = mq_receive(mq_d, (char *)pkg, sizeof(*pkg), 0);
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        perror("mq_receive");
        return;
    }
}



void* create_multiple_clientfd(void *arg) {

    client_info *table = (client_info *)arg;

    for (int idx = 0; idx < CLI_MAX; idx ++) {

        int res = client_connect_init(&table[idx].s_info);
        if (res < 0) {
            printf("connect failed\n");
            return NULL;
        } else {
            printf("[%d]fd:%d\n", idx, table[idx].s_info.fd);

            //register msgqueue for receive notification messages from server
            int res = msgqueue_req(MSGQ_LINK, O_NONBLOCK | O_RDONLY, 0, &table[idx].m_info);
            if (res < 0) {
                printf("error %d\n", res);
                close(table[idx].s_info.fd);
                mq_close(table[idx].m_info.mq_d);
            } else {
                table[idx].in_use = true;
                register_callback_func(&table[idx], confrim_connect);
                table[idx].m_info.notify_callback(table[idx].m_info.mq_d, &table[idx].pkg);
            }

        }

    }
    while (1) {

        printf("wait...\n");
        sleep(10);

    }
    return NULL;
}





int main() {

    int m_res = pthread_mutex_init(&mutex, NULL);
    if (m_res != 0) {
        perror("Mutex initialization failed");
    }


    int p1_res = pthread_create(&pid, NULL, create_multiple_clientfd, (client_info *)list);
    if (p1_res != 0) {
        perror("Thread 2 create failed");
    }

    pthread_join(pid, NULL);
/*
    for (int idx = 0; idx < CLI_MAX; idx ++) {
        close(list[idx].s_info.fd);
    }
*/

    return 0;

}
