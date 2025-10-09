
#include "IPC_SOCK_config.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
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

#include "IPC_SOCK_general.h"


static sock_info server;

static client_info client_list[CLI_MAX] = {0};

static struct epoll_event events[MONITOR_MAX] = {0};

static int epoll_fd;

static pthread_t pid;


void connect_notify(mqd_t mq_d, package *pkg) {

    const char *msg = "[Server]: Connection request received.";
    strcpy(pkg->msg, msg);
    pkg->payload_len = sizeof(*msg);

    int res = mq_send(mq_d, (const char*)pkg, sizeof(*pkg), 0);
    if (res < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        perror("mq_send");
        mq_close(mq_d);
        return;
    }
    printf("(%s) ok!\n", __func__);
}



/* received msg */
void recv_task(struct epoll_event *ev) {

    char buff[MSG_MAX] = {0};

    ssize_t bytes = recv(ev->data.fd, buff, MSG_MAX, MSG_DONTWAIT);
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("read finished\n");
            return;
        }
        perror("recv");
        epoll_delete(epoll_fd, ev);
        close(ev->data.fd);
    } else if (bytes > 0) {
        printf("fd(%d) Recv msg: %s\n", ev->data.fd, buff);
        memset(buff, 0, sizeof(buff));
    } else if (bytes == 0) {
        printf("fd(%d) disconnected\n", ev->data.fd);
    }

}


void accept_task(client_info *table) {

    while (1) {

        int c_fd = accept(server.fd, NULL, NULL);
        if (c_fd < 0) {

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            perror("accept4");
            break;
        }


        client_info *ptr = find_Space_or_Member(0, false, table);
        if (ptr == NULL) {
            printf("no space in client_list, is full !!\n");
            close(c_fd);
            break;
        } else {
            ptr->in_use = true;
            ptr->s_info.fd = c_fd;

            // setting NONBLOCK
            set_nonblocking(ptr->s_info.fd);

            // fd add to epoll interest list
            epoll_add(epoll_fd, &ptr->s_info);
        }

        //register msgququ for something status to notify client

        //int flag = O_CREAT | O_EXCL | O_WRONLY;
        //printf("flag %d\n", flag);

        int q_res = msgqueue_req(MSGQ_LINK, O_CREAT | O_EXCL | O_NONBLOCK | O_WRONLY, 0600, &ptr->m_info);
        if (q_res < 0) {
            perror("mq_open");
            //epoll_delete(epoll_fd, &ptr->s_info.ev);
            //close(c_fd);
            //memset(ptr, 0, sizeof(client_info));
            //continue;
        } else {
            printf("mq_d %d\n", ptr->m_info.mq_d);
            register_callback_func(ptr, connect_notify);
            ptr->m_info.notify_callback(ptr->m_info.mq_d, &ptr->pkg);
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
                    recv_task(&events[idx]);
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
    epoll_add(epoll_fd, &server);

    //epoll_monitor
    int p_res = pthread_create(&pid, NULL, epoll_monitor, (client_info *)&client_list);
    if (p_res < 0) {
        perror("pthread_create");
    }

    for (int i = 0; i < MONITOR_MAX; i++) {
        printf("wait[%d] fd: %d\n", i, events[i].data.fd);
    }
    pthread_join(pid, NULL);

    //DELETE ALL REGISTER FD FROM INTEREST LIST
    //CLOSE SERVER
    printf("check\n");
    return 0;
}

