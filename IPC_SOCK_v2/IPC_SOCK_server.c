
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

#include "IPC_SOCK_common.h"
#include "IPC_SOCK_server.h"

/* select connection type */
#define IDS_CONNECT


#define CLI_MAX   5


static sock_info server;

static client_info client_info_list[CLI_MAX] = {0};

static pthread_t pid_1;
static pthread_t pid_2;

static int stopAll_flag = 1;

static package global_pkg = {0}; //for test race conditions

static pthread_mutex_t mutex;

void* recv_msg_from_client(void *arg) {

    while (stopAll_flag) {

        client_info *ptr = (client_info *)arg;
        package temp = {0};

        if (!ptr->start_flag) {
            break;
        } else if (ptr->start_flag) {

            ssize_t r_bytes = recv(ptr->client.fd, &temp, sizeof(package), 0);
            if (r_bytes == -1) {
                perror("Recv error");
                break;
            } else if (r_bytes > 0) {
                lock(&mutex, __func__);
                memcpy(&ptr->pkg, &temp, sizeof(package));
                unlock(&mutex, __func__);

                printf("fd %d mag:%s (bytes:%zd)\n", ptr->client.fd, ptr->pkg.msg, r_bytes);
            } else if (r_bytes == 0) {
                //disconnected
                error_handler(ptr);
                return NULL;
            }
        }
    }
    return NULL;
}



void* send_msg_to_client(void *arg) {

    while (stopAll_flag) {

        client_info *ptr = (client_info *)arg;

        if (!ptr->start_flag) {
            break;
        } else if (ptr->start_flag) {
            sleep(1);
            ssize_t s_bytes = send(ptr->client.fd, &ptr->pkg, sizeof(package), 0);
            if (s_bytes == -1) {
                perror("Send error");
                break;
            }
            //printf("(%s) fd %d, msg: %s\n", __func__, ptr->client.fd, ptr->pkg.msg);
        }
    }
    return NULL;
}

static client_info* find_client_space(client_info *list) {

    for (int idx = 0; idx < CLI_MAX; idx ++) {

        if (!list[idx].in_use) {
            return &list[idx];
        } else if (list[idx].in_use) {
            continue;
        }
    }
    return NULL;
}

/* if create thread fail, it will stop (join_) thread and (close_) fd */
void* monitor_connect_task(void *arg) {

    while (stopAll_flag) {
        int fd = accept(*(int *)arg, NULL, NULL);
        if (fd < 0) {
            perror("Accept error");
            return NULL;
        } else if (fd > 0) {
            client_info *space = find_client_space(client_info_list);
            if (!space) {
                printf("Array full\n");
                continue;
            }
            space->client.fd = fd;
            space->start_flag = 1;
            int recv_res = pthread_create(&space->r_pid, NULL, recv_msg_from_client, (client_info *)space);
            int send_res = pthread_create(&space->s_pid, NULL, send_msg_to_client, (client_info *)space);
            if (recv_res != 0 || send_res != 0) {
                printf("fd %d recv or send thread create failed....\n", space->client.fd);
                error_handler(space);
                return NULL;
            }
            space->in_use = true;
            //printf("fd %d (%s)\n", space->client.fd, __func__);
            //addr info .....
        }
    }
    printf("%s\n", __func__);
    return NULL;
}

void stop_listen(int *listen_fd) {

    shutdown(*listen_fd, SHUT_RDWR);
    close(*listen_fd);

}

void* monitor_disconnect_task(void *arg) {

    client_info *ptr = (client_info *)arg;
    static int count = 0;

    printf("waiting for client to connect within 5 seconds....\n");

    while (stopAll_flag) {

        sleep(5);
        for (int idx = 0; idx < CLI_MAX; idx ++) {
            if (!ptr[idx].start_flag) {
                count ++;
            }
        }

        if (count == CLI_MAX) {
            printf("prepare to close the main thread\n");
            stopAll_flag = 0;
            stop_listen(&server.fd);
        } else if (count < CLI_MAX) {
            //reset
            count = 0;
        }
    }
    printf("%s\n", __func__);
    return NULL;
}



/* defult : UDS connect */
/* if want select IDS connect -> please '#define IDS_CONNECT' */
int server_connect_init(sock_info *server) {

#ifdef IDS_CONNECT

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->fd == -1) {
        perror("Socket error");
        return -1;
    } else {
        printf("Socket[IDS] successfully created.\n");
        bzero(&server->addr.ids, sizeof(server->addr.ids));
    }

    printf("socket fd %d\n", server->fd);

    server->addr.ids.sin_family = AF_INET;
    server->addr.ids.sin_port = htons(PORT);
    server->addr.ids.sin_addr.s_addr = htonl(INADDR_ANY);

    server->len = sizeof(struct sockaddr_in);

    int bind1_res = bind(server->fd, (struct sockaddr *)&server->addr.ids, server->len);
    if (bind1_res == -1) {
        perror("Bind error");
        return -2;
    }


    int listen1_res = listen(server->fd, 5);
    if (listen1_res == -1) {
        perror("Listen error");
        return -3;
    }

#else

    server->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server->fd == -1) {
        perror("Socket error");
        return -1;
    } else {
        printf("Socket[UDS] successfully created.\n");
    }

    printf("socket fd %d\n", server->fd);

    unlink(PATH);
    server->addr.uds.sun_family = AF_UNIX;
    strcpy(server->addr.uds.sun_path, PATH);
    server->len = sizeof(struct sockaddr_un);

    int bind2_res = bind(server->fd, (struct sockaddr *)&server->addr.uds, server->len);
    if (bind2_res == -1) {
        perror("Bind error");
        return -2;
    }

    printf("server path: %s\n", server->addr.uds.sun_path);

    int listen2_res = listen(server->fd, 5);
    if (listen2_res == -1) {
        perror("Listen error");
        return -3;
    }

#endif

    return 0;
}


int set_nonblocking(int client_fd) {

    int flag = fcntl(client_fd, F_GETFL, 0);
    if (flag < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }

    if (fcntl(client_fd, F_SETFL, flag | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL)");
        return -2;
    }

    return 0;
}




int main() {


    int sock_init = server_connect_init(&server);
    if (sock_init < 0) {
        printf("Socket init failed.\n");
    }

    //Mutex
    int m_res = pthread_mutex_init(&mutex, NULL);
    if (m_res != 0) {
        perror("Mutex initialization failed");
    }






    int efd = epoll_create1(EPOLL_CLOEXEC);
    if (efd < 0) {
        perror("epoll_create1");
        return -1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server.fd;

    int c_res = epoll_ctl(efd, EPOLL_CTL_ADD, server.fd, &ev);
    if (c_res < 0) {
        perror("epoll_ctl");
        return -2;
    }


    struct epoll_event events[10];

    while (1) {
        int n_events = epoll_wait(efd, events, 10, -1);
        if (n_events < 0) {
            perror("epoll_wait");
            break;
        }

        for (int idx = 0; idx < n_events; idx++) {

            if (events[idx].data.fd == server.fd) {

                while (1) {

                    struct sockaddr_in adr;
                    socklen_t len = sizeof(adr);
                    int client_fd = accept(server.fd, (struct sockaddr *)&adr, &len);
                    if (client_fd < 0) {
                        perror("accept");
                    }

                    //set new client socket to non-blocking
                    set_nonblocking(client_fd);

                    //add new client socket to the epoll interest list
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = client_fd;
                    if (epoll_ctl(efd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                        perror("epoll_ctl");
                        close(client_fd);
                    }
                }

            } else {

                //data from an existing client
                int client_fd = events[idx].data.fd;
                char buff[1024];

                //read data from client
                ssize_t count = read(client_fd, buff, sizeof(buff));
                while (count > 0) {
                    write(STDOUT_FILENO, buff, count);
                }

                if (count == 0) {
                    printf("Client %d disconnected.\n", client_fd);
                    close(client_fd);
                } else if (count < 0) {
                    //EAGAIN ??
                    if (errno != EAGAIN) {
                        perror("read");
                        close(client_fd);
                    }
                }

            }

        }
    }




/*


    //Thread_1
    int p1_res = pthread_create(&pid_1, NULL, monitor_connect_task, (int *)&server.fd);
    if (p1_res != 0) {
        perror("monitor_connect_task thread create failed");
    }

    //Thread_2
    int p2_res = pthread_create(&pid_2, NULL, monitor_disconnect_task, (client_info *)client_info_list);
    if (p2_res != 0) {
        perror("monitor_connect_task thread create failed");
    }

    pthread_join(pid_2, NULL);
    pthread_join(pid_1, NULL);
*/


    printf("%s\n", __func__);
    return 0;

}
