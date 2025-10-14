
#include "IPC_SOCK_config.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <bits/time.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
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



void* final_connect_check(void *arg) {

    client_info *ptr = (client_info *)arg;

    unsigned int prio = 0;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 10;

    while (1) {

        lock(&mutex, __func__);
        memset(&ptr->m_info.m_pkg.msg, 0, sizeof(ptr->m_info.m_pkg.msg));
        unlock(&mutex, __func__);

        ssize_t r_byte = mq_timedreceive(ptr->m_info.mq_d, (char *)&ptr->m_info.m_pkg.msg,
                                         sizeof(ptr->m_info.m_pkg.msg), &prio, &ts);
        if (r_byte < 0){
            perror("mq_timedreceive");
            break;
        }

        printf("%s() recv msg: %s\n", __func__, ptr->m_info.m_pkg.msg);

        //buff[strcspn(buff, "\r\n")] = '\0';
        if (strcmp(ptr->m_info.m_pkg.msg, "[ACK]") == 0) {

            ptr->m_info.conn_status = 1;

            printf ("%s() success connection (set conn_status) \n", __func__);
            break;
        }
    }
    return NULL;
}



void register_messageQueue(client_info *ptr) {


    set_nonblocking(ptr->s_info.fd);

    //waiting for the server to send MSG Queue 'link'
    while (1) {
        ssize_t r_byte = recv(ptr->s_info.fd, &ptr->m_info.link, 11, MSG_DONTWAIT);
        if (r_byte < 0) {

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                //printf("hello....\n");
                break;
            }

            perror("recv");
            break;
        }
        printf("%s() received link '%s'\n", __func__, ptr->m_info.link);

        int mq = msgqueue_req(ptr->m_info.link, O_NONBLOCK | O_RDWR, 0, &ptr->m_info.mq_d, &ptr->m_info.mq_att);
        if (mq < 0) {
            close(ptr->s_info.fd);
            printf("%s() fd %d msgqueue_req error\n", __func__, ptr->s_info.fd);
            break;
        }

        epoll_add(epoll_fd, ptr->m_info.mq_d, &ptr->m_info.m_ev);

        char msg[15] = "[SYN/ACK]";
        package temp = {0};

        lock(&mutex, __func__);
        strcpy(temp.msg, msg);
        memset(&temp, 0, sizeof(package));
        unlock(&mutex, __func__);


        int s_res = mq_send(ptr->m_info.mq_d, (const char *)&temp.msg,
                                 sizeof(temp.msg), 0);
        if (s_res < 0) {
            if (errno != EAGAIN || errno != EWOULDBLOCK) {
                perror("send");
                break;
            }
        }


        printf("sent %s to server ... waiting for server response[ACK]\n", temp.msg);

    }

}


void create_mutiple_client(client_info *table) {

    for (int idx = 0; idx < CLI_MAX; idx ++) {

        int res = client_connect_init(&table[idx].s_info);
        if (res < 0) {
            printf("%s() connect init failed\n", __func__);
            continue;
        }
        table[idx].in_use = true;
        printf("%s() fd %d\n", __func__, table[idx].s_info.fd);

        epoll_add(epoll_fd, table[idx].s_info.fd, &table[idx].s_info.s_ev);
    }

}



void* client_epoll_monitor(void *arg) {

    client_info *table = (client_info *)arg;

    //create client socket and add to epoll interest list
    create_mutiple_client(table);

    while (1) {

        int n_evn = epoll_wait(epoll_fd, epoll_evns, MONITOR_MAX, -1);
        if (n_evn < 0) {
            perror("epoll_wait");
            break;
        }

        printf("check wait num: %d\n", n_evn);

        int fd = 0;
        for (int idx = 0; idx < MONITOR_MAX; idx ++) {
            fd = epoll_evns[idx].data.fd;

            client_info *temp = find_Space_or_Member(fd, true, list);
            if (!temp) {
                printf("%s(): not found fd/mq_d in client list\n", __func__);
                break;
            }
            printf("%p\n", temp);

            if (!temp->m_info.conn_status) {
                //connection initialization has not been completed

                if (fd == temp->s_info.fd) {
                    //server send 'link' for client register messageQueue
                    register_messageQueue(temp);

                } else if (fd == temp->m_info.mq_d) {
                    //waiting for the final server [ACK]
                    //to setting connect flag == 1

                    int pres2 = pthread_create(&temp->detach_pid, NULL, final_connect_check,
                                               (client_info *)temp);
                    if (pres2 < 0) {
                        perror("pthread_create(2)");
                        continue;
                    }
                    pthread_detach(temp->detach_pid);

                }


            } else if (temp->m_info.conn_status) {
                //normal receive

                if (fd == temp->s_info.fd) {
                    //rece_task
                } else if (fd == temp->m_info.mq_d) {
                    //mqrece_task
                }
            }

        }

    //show epoll wait event list
    for (int i = 0; i < MONITOR_MAX; i++) {
            printf("wait[%d] fd: %d\n", i, epoll_evns[i].data.fd);
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
