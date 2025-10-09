
#include "IPC_SOCK_config.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
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



/* new client connect */
void accept_task(int serfd) {

    while (1) {

        int c_fd = accept(serfd, NULL, NULL);
        if (c_fd < 0) {
            //check error
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            perror("accept4");
            break;
        }

        //add info to list
        client_info *space = add_member(client_list);
        if (space == NULL) {
            printf("buff is full");
            close(c_fd);
            return;
        }
        space->in_use = true;
        space->s_info.fd = c_fd;

        set_nonblocking(space->s_info.fd);
        epoll_add(epoll_fd, &space->s_info);


    }

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
        release_member(ev->data.fd, client_list);
        epoll_delete(epoll_fd, ev);
        close(ev->data.fd);
    }

}


void* epoll_monitor(void *arg) {

    int *serfd = (int *)arg;

    while (1) {

        int n_evn = epoll_wait(epoll_fd, events, MONITOR_MAX, -1);
        if (n_evn < 0) {
            perror("epoll_wait");
            break;
        }
        printf("check wait num: %d\n", n_evn);

        int fd = 0;
        for (int idx = 0; idx < MONITOR_MAX; idx ++) {
            fd = events[idx].data.fd;
            if (fd == *serfd) {
                //new client
                accept_task(*serfd);
            } else {
                //old client
                uint32_t even = events[idx].events;
                //check events --> call recv/send/error
                if (even & EPOLLIN) {
                    recv_task(&events[idx]);
                } else if (even & EPOLLOUT) {
                    //send_task
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
    int p_res = pthread_create(&pid, NULL, epoll_monitor, (int *)&server.fd);
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

