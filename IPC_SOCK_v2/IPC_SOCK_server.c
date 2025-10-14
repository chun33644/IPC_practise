
#include "IPC_SOCK_config.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <bits/types/sigevent_t.h>
#include <mqueue.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <sys/un.h>
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


void mq_connect_check(union sigval sv) {

    client_info *temp = (client_info *)sv.sival_ptr;
    printf("%s()\n", __func__);

    unsigned int prio = 0;

    while (1) {

        memset(&temp->m_info.m_pkg, 0, sizeof(temp->m_info.m_pkg));
        ssize_t r_byte = mq_receive(temp->m_info.mq_d, (char *)&temp->m_info.m_pkg.msg, sizeof(temp->m_info.m_pkg.msg), &prio);
        if (r_byte < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Hello....\n");
                break;
            }
            perror("mq_receive");
            break;
        }


        //buff[strcspn(buff, "\r\n")] = '\0';
        if (strcmp(temp->m_info.m_pkg.msg, "[SYN/ACK]") == 0) {

            printf("%s() recv msg: %s\n", __func__, temp->m_info.m_pkg.msg);

            temp->m_info.conn_status = 1;
            printf("%s() setting conn_status OK!\n", __func__);

            char *msg = "[ACK]";
            lock(&mutex, __func__);
            memset(&temp->m_info.m_pkg, 0, sizeof(temp->m_info.m_pkg));
            strcpy(temp->m_info.m_pkg.msg, msg);
            unlock(&mutex, __func__);

            int s_res = mq_send(temp->m_info.mq_d, (const char *)&temp->m_info.m_pkg.msg, sizeof(temp->m_info.m_pkg.msg), 0);
            if (s_res < 0) {
                if (errno != EAGAIN || errno != EWOULDBLOCK) {
                    perror("mq_send");
                    break;
                }
            }
            printf("%s() sent %s to client\n", __func__, temp->m_info.m_pkg.msg);
            return;

        } else {
            printf("%s() [SYN/ACK] error\n", __func__);
        }

    }

}


void* msgq_connect_monitor(void *arg) {

    printf("%s()\n", __func__);

    client_info *temp = (client_info *)arg;


    while (1) {

        if (!temp->m_info.conn_status) {

            //printf("%s() 'OK' send by client has not been received yet\n", __func__);
            //sleep(2);
            continue;

        } else {

            client_info *ptr = find_Space_or_Member(0, false, client_list);
            if (ptr == NULL) {
                printf("no space in client_list, is full !!\n");
                close(temp->s_info.fd);
                mq_unlink(temp->m_info.link);
                free(temp);
                break;
            } else {
                memcpy(ptr, temp, sizeof(client_info));
                ptr->in_use = true;

                // fd add to epoll interest list
                epoll_add(epoll_fd, temp->s_info.fd, &temp->s_info.s_ev);
                epoll_add(epoll_fd, temp->m_info.mq_d, &temp->m_info.m_ev);
            }

            printf("%s() client %d complete the initial connection settings.\n", __func__, ptr->s_info.fd);
            break;
        }

    }

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

        set_nonblocking(c_fd);

        client_info *temp = malloc(sizeof(client_info));
        if (!temp) {
            printf("malloc struct client_info failed\n");
            free(temp);
            break;
        }
        memset(temp, 0, sizeof(*temp));
        temp->s_info.fd = c_fd;

        //register msgququ for something status to notify client
        int len = snprintf(temp->m_info.link, MSG_MAX,
                 "/MSGQLINK_%d",
                 temp->s_info.fd);


        int q_res = msgqueue_req(temp->m_info.link, O_CREAT | O_EXCL | O_NONBLOCK | O_RDWR, 0600, &temp->m_info.mq_d, &temp->m_info.mq_att);
        if (q_res < 0) {

            close(temp->s_info.fd);
            free(temp);
            break;

        } else {

            ssize_t s_byte = send(temp->s_info.fd, temp->m_info.link, (ssize_t)len, MSG_DONTWAIT);
            if (s_byte < 0) {
                if (errno != EAGAIN || errno != EWOULDBLOCK) {
                    perror("send");
                    close(temp->s_info.fd);
                    mq_unlink(temp->m_info.link);
                    free(temp);
                    break;
                }
            }

            printf("%s(): send link '%s' to client\n", __func__, temp->m_info.link);

            struct sigevent sev;
            sev.sigev_notify = SIGEV_THREAD;
            sev.sigev_notify_function = mq_connect_check;
            sev.sigev_notify_attributes = NULL;
            sev.sigev_value.sival_ptr = temp;

            int nr = mq_notify(temp->m_info.mq_d, &sev);
            if (nr < 0) {
                perror("mq_notify");
                break;
            }

/*
            int p_res = pthread_create(&temp->detach_pid, NULL, msgq_connect_monitor, (client_info *)temp);
            if (p_res < 0) {
                perror("pthread_create");
                close(temp->s_info.fd);
                mq_unlink(temp->m_info.link);
                free(temp);
                break;
            }
            pthread_detach(temp->detach_pid);
*/
        }

    }

}




void* epoll_monitor(void *arg) {

    client_info *c_table = (client_info *)arg;

    while (1) {

        int n_evn = epoll_wait(epoll_fd, events, MONITOR_MAX, -1);
        if (n_evn < 0) {
            perror("epoll_wait");
            break;
        }
        //printf("check wait num: %d\n", n_evn);

        int fd = 0;
        for (int idx = 0; idx < MONITOR_MAX; idx ++) {
            fd = events[idx].data.fd;
            if (fd == server.fd) {
               accept_task(c_table);
            } else {
                uint32_t even = events[idx].events;

                if (even & EPOLLIN) {
                    //recv_task
                } else if (even & EPOLLOUT) {
                    //send_task
                } else if (even & EPOLLRDHUP) {
                    //disconnected
                    epoll_delete(epoll_fd, &events[idx]);
                    close(events[idx].data.fd);
                }
            }
        }

        for (int i = 0; i < MONITOR_MAX; i++) {
            printf("wait[%d] fd: %d\n", i, events[i].data.fd);
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
    epoll_add(epoll_fd, server.fd, &server.s_ev);

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

