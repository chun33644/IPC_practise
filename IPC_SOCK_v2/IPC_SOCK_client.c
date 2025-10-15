
#include "IPC_SOCK_config.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <bits/time.h>
#include <bits/types/__sigval_t.h>
#include <bits/types/sigevent_t.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "IPC_SOCK_general.h"

static client_info list[CLI_MAX] = {0};

static pthread_mutex_t mutex;

static pthread_t pid;

static int epoll_fd;

static struct epoll_event epoll_evns[MONITOR_MAX] = {0};



void msgq_ack_confirm(union sigval sv){

    client_info * ptr = (client_info *)sv.sival_ptr;

    unsigned int prio = 0;
    char ack[MSG_MAX] = {0};

    while(1) {

        ssize_t rbyte = mq_receive(ptr->m_info.mq_d, ack, sizeof(ack), &prio);
        if (rbyte < 0) {
            perror("mq_receive[ACK]");
            break;
        }

        //printf("%s() received '%s'\n", __func__, ack);
        if (strcmp(ack, "[ACK]") == 0) {
            memset(ack, 0, sizeof(ack));

            ptr->m_info.conn_status = true;
            ptr->s_info.s_ev.events = EPOLLIN | EPOLLET;
            ptr->s_info.s_ev.data.ptr = ptr;

            int ectl = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ptr->s_info.fd, &ptr->s_info.s_ev);
            if (ectl < 0) {
                perror("epoll_ctl(remove EPOLLONESHOT)");
                break;
            }
            printf("%s() fd %d completed the init connection setting.\n", __func__, ptr->s_info.fd);
        } else {
            continue;
        }

    }

    printf("%s() connection init failed, will be close fd and mqd.......\n", __func__);

    close(ptr->s_info.fd);
    mq_close(ptr->m_info.mq_d);
    release_member(ptr);

}


void register_messageQueue(client_info *ptr) {


    while (1) {

        //waiting for the server to send MSG Queue 'link'
        ssize_t r_byte = recv(ptr->s_info.fd, &ptr->m_info.link, sizeof(ptr->m_info.link), 0);
        if (r_byte < 0) {

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }

            perror("recv(msgq link)");
            break;
        }
        printf("%s() received msg link '%s'\n", __func__, ptr->m_info.link);

        int mq = msgqueue_req(ptr->m_info.link, O_RDWR, 0, &ptr->m_info.mq_d, &ptr->m_info.mq_att);
        if (mq < 0) {
            printf("%s() fd %d create error\n", __func__, ptr->s_info.fd);
            break;
        }

        struct sigevent sev;
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_notify_function = msgq_ack_confirm;
        sev.sigev_value.sival_ptr = ptr;
        sev.sigev_notify_attributes = NULL;

        //final check whether receive '[ACK]' response from server
        //if don't receive '[ACK]',
        //epoll no more events will be notified and connect flag will always be 'false'......
        int nres = mq_notify(ptr->m_info.mq_d, &sev);
        if (nres < 0) {
            perror("mq_notify");
            break;
        }

        char msg[15] = "[SYN/ACK]";
        int s_res = mq_send(ptr->m_info.mq_d, msg, sizeof(msg), 0);
        if (s_res < 0) {
            perror("send");
            break;
        }



        //printf("sent '%s' to server ... waiting for server response[ACK]\n",msg);


        return;

    }

    close(ptr->s_info.fd);
    mq_close(ptr->m_info.mq_d);
    release_member(ptr);

}

void create_multiple_client(client_info *table) {


    for (int idx = 0; idx < CLI_MAX ; idx ++) {

        int res = client_connect_init(&table[idx].s_info);
        if (res < 0) {
            printf("%s() failed\n", __func__);
            continue;
        }
        table[idx].in_use = true;
        //printf("%s() fd %d\n", __func__, table[idx].s_info.fd);

        set_nonblocking(table[idx].s_info.fd);
        /* setting one-shot notification, change the setting after waiting for connection init completed */
        table[idx].s_info.s_ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        table[idx].s_info.s_ev.data.ptr = &table[idx];
        epoll_add(epoll_fd, table[idx].s_info.fd, table[idx].s_info.s_ev);
    }

}



void* client_epoll_monitor(void *arg) {

    client_info *table = (client_info *)arg;

    //create client socket and add to epoll interest list
    create_multiple_client(table);

    while (1) {

        int n = epoll_wait(epoll_fd, epoll_evns, MONITOR_MAX, -1);
        if (n < 0) {
            perror("epoll_wait");
            break;
        }

        client_info *ptr = NULL;
        for (int idx = 0; idx < n; idx ++) {
            ptr = epoll_evns[idx].data.ptr;

            if (!ptr->m_info.conn_status) {
                /* connection initialization has not been completed */
                printf("%s() client %-2d enter register_messageQueue()....\n", __func__, ptr->s_info.fd);
                register_messageQueue(ptr);

            } else if (ptr->m_info.conn_status) {
                /* normal event task */
                uint32_t even = epoll_evns[idx].events;
                printf("%s() client %-2d event trigger 0x%08x\n", __func__, ptr->s_info.fd, even);
                if (even & EPOLLIN) {
                    //receive_task
                }
            }

        }
    }

    return NULL;
}



int main() {

    int m_res = pthread_mutex_init(&mutex, NULL);
    if (m_res != 0) {
        perror("Mutex initialization failed");
    }


    epoll_req(&epoll_fd);


    int p1_res = pthread_create(&pid, NULL, client_epoll_monitor, (client_info *)list);
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
