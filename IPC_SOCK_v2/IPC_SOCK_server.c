
#include "IPC_SOCK_config.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <bits/time.h>
#include <bits/types/sigevent_t.h>
#include <mqueue.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#include "IPC_SOCK_general.h"


static sock_info server;

static client_info client_list[CLI_MAX] = {0};

static struct epoll_event events[MONITOR_MAX] = {0};

static int epoll_fd;

static pthread_t pid;

static pthread_mutex_t mutex;


void* msgq_connect_handshake(void *arg){

    client_info *ptr = (client_info *)arg;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 10;

    unsigned int prio = 0;
    int start = 0;

    int qres = msgqueue_req(ptr->m_info.link, O_CREAT | O_EXCL | O_RDWR, 0600, &ptr->m_info.mq_d, &ptr->m_info.mq_att);
    if (qres < 0) {
        printf("%s() message queue open error\n", __func__);
    }
    start = 1;

    while (start) {

        ssize_t s = send(ptr->s_info.fd, ptr->m_info.link, sizeof(ptr->m_info.link), 0);
        if (s < 0) {
            perror("send(msgq link)");
            break;
        }
        //printf("%s() sent link '%s' to client fd %d\n", __func__, ptr->m_info.link, ptr->s_info.fd);


        char synack[MSG_MAX] = {0};
        ssize_t r = mq_timedreceive(ptr->m_info.mq_d, synack, sizeof(synack), &prio, &ts);
        if (r < 0) {
            if (errno == ETIMEDOUT) {
                printf("%s() mq_timedreceive timeout......\n", __func__);
                break;
            }
            perror("mq_timedreceive[SYN/ACK]");
            break;
        }

        //printf("%s() received '%s'\n", __func__, synack);

        //buff[strcspn(buff, "\r\n")] = '\0';
        if (strcmp(synack, "[SYN/ACK]") == 0) {

            ptr->m_info.conn_status = 1;

            set_nonblocking(ptr->s_info.fd);
            set_nonblocking(ptr->m_info.mq_d);

            /* fd add to epoll interest list */
            ptr->s_info.s_ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
            ptr->s_info.s_ev.data.ptr = ptr;
            epoll_add(epoll_fd, ptr->s_info.fd, ptr->s_info.s_ev);
            //epoll_add(epoll_fd, ptr->m_info.mq_d, ptr->m_info.m_ev);


            char *msg = "[ACK]";
            size_t len = strlen(msg);
            int s = mq_send(ptr->m_info.mq_d, msg, len, 0);
            if (s < 0) {
                perror("mq_send[ACK]");
                break;
            }

            printf("%s() client %-2d complete the initial connection settings.\n", __func__, ptr->s_info.fd);
            return NULL;
        }
        break;
    }

    close(ptr->s_info.fd);
    mq_close(ptr->m_info.mq_d);
    mq_unlink(ptr->m_info.link);
    release_member(ptr);
    printf("%s() close and unlink msg queue.....\n", __func__);
    return NULL;


}



void accept_task(client_info *table) {

    while (1) {

        int c_fd = accept(server.fd, NULL, NULL);
        if (c_fd < 0) {

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }

            if (errno == EINTR) {
                continue;
            }

            perror("accept4");
            break;
        }

        client_info *space = find_Space_or_Member(c_fd, false, table);
        if (!space) {
            printf("%s() no space in client list\n", __func__);
            close(c_fd);
            continue;
        }
        memset(space, 0, sizeof(client_info));
        space->s_info.fd = c_fd;
        space->in_use = true;
        int len = snprintf(space->m_info.link, MSGQ_LINK_SIZE, "/MSGQLINK_%d", space->s_info.fd);

        int pres = pthread_create(&space->detach_pid, NULL, msgq_connect_handshake, (client_info *)space);
        if (pres < 0){
            perror("pthread_create(msgq_connect_handshake)");
            release_member(space);
            close(space->s_info.fd);
            continue;
        }
        pthread_detach(space->detach_pid);

    }

}



void* epoll_monitor(void *arg) {

    client_info *c_table = (client_info *)arg;

    while (1) {

        int n = epoll_wait(epoll_fd, events, MONITOR_MAX, -1);
        if (n < 0) {
            perror("epoll_wait");
            break;
        }
        //printf("check wait num: %d\n", n);

        client_info *ptr = NULL;
        int fd = 0;
        uint32_t even = 0;
        for (int idx = 0; idx < n; idx ++) {
            fd = events[idx].data.fd;
            if (fd == server.fd) {
                /* new client */
                printf("%s() new client %d !\n", __func__, fd);

                accept_task(c_table);

            } else {
                ptr = events[idx].data.ptr;
                even = events[idx].events;

                printf("%s() client %-2d event trigger 0x%08x\n", __func__, ptr->s_info.fd, even);

                if (even & EPOLLRDHUP) {
                    //disconnected
                    epoll_delete(epoll_fd, ptr->s_info.fd);
                    close(ptr->s_info.fd);
                    mq_unlink(ptr->m_info.link);
                    printf("%s() client %-2d ---- offline (removed all info) ----\n", __func__, ptr->s_info.fd);
                    continue;
                }
                if (even & EPOLLIN) {
                    //received_task
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


    //sock_create
    int sock_init = server_connect_init(&server);
    if (sock_init < 0) {
        printf("Socket init failed.\n");
    }

    //set_nonblock --> (server.fd)
    set_nonblocking(server.fd);

    //request a epoll fd
    epoll_req(&epoll_fd);

    //epoll_add --->(server.fd) ... add to epoll interest list
    server.s_ev.events = EPOLLIN | EPOLLET;
    server.s_ev.data.fd = server.fd;
    epoll_add(epoll_fd, server.fd, server.s_ev);

    //epoll_monitor
    int p_res = pthread_create(&pid, NULL, epoll_monitor, (client_info *)&client_list);
    if (p_res < 0) {
        perror("pthread_create");
    }
    pthread_join(pid, NULL);

    //DELETE ALL REGISTER FD FROM INTEREST LIST
    close(server.fd);
    printf("check\n");
    return 0;
}

