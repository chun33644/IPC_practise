
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "IPC_SOCK_general.h"
#include "IPC_SOCK_config.h"

/* mutex lock */
int lock(pthread_mutex_t *mutex, const char *func_name) {

    int res = pthread_mutex_lock(mutex);
    if (res != 0) {
        printf("lock error (%s)\n", func_name);
    }

    //printf("%s lock\n", func_name);
    return 0;
}


/* mutex unlock */
int unlock(pthread_mutex_t *mutex, const char *func_name) {
    int res = pthread_mutex_unlock(mutex);
    if (res != 0) {
        printf("unlock error (%s)\n", func_name);
    }

    //printf("%s unlock\n", func_name);
    return 0;
}


/* closed thread and fd */
int error_handler(client_info *list) {

    list->start_flag = 0;
    printf("Thread ready to join\n");
    pthread_join(list->rev_pid, NULL);
    pthread_join(list->sen_pid, NULL);

    close(list->s_info.fd);
    memset(list, 0, sizeof(client_info));

    return 0;
}


/* find (in_use == false) from array for management list */
client_info* add_member(client_info *list) {

    for (int idx = 0; idx < CLI_MAX; idx ++) {

        if (!list[idx].in_use) {
            return &list[idx];
        } else if (list[idx].in_use) {
            continue;
        }

    }
    return NULL;
}


/* release from management list */
int release_member(int fd, client_info *list) {

    for(int idx = 0; idx < CLI_MAX; idx ++) {

        if (fd != list[idx].s_info.fd) {
            continue;
        } else {
            list[idx].in_use = false;
            printf("fd(%d) ready to release from list.\n", list->s_info.fd);
            memset(&list[idx], 0, sizeof(list[idx]));
            return 0;
        }

    }
    printf("delete fail\n");
    return -1;
}


/* setting fd flag (O_NONBLOCK) */
int set_nonblocking(int fd) {

    int flag = fcntl(fd, F_GETFL, 0);
    if (flag < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }

    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL)");
        return -2;
    }

    return 0;
}

/* register epoll fd */
int epoll_req(int *new_efd) {

    *new_efd = epoll_create1(EPOLL_CLOEXEC);
    if (new_efd < 0) {
        perror("epoll_create");
        return -1;
    }
    printf("epoll fd %d\n", *new_efd);

    return 0;
}

/* epoll fd and listen fd add to interst list */
int epoll_add(int epoll_fd, sock_info *info) {

    info->ev.events = EPOLLIN | EPOLLET;
    info->ev.data.fd = info->fd;

    int c_res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, info->fd, &info->ev);
    if (c_res < 0) {
        perror("epoll_ctl(EPOLL_CTL_ADD)");
        close(info->fd);
        return -1;
    }

    printf("fd(%d) added to epoll interst list.\n", info->fd);
    return 0;
}


/* delete fd from interst list */
int epoll_delete(int epoll_fd, struct epoll_event *close_ev) {

    if (!close_ev->data.fd) {
        printf("add to epoll not yet.\n");
        return -1;
    }

    printf("fd(%d) ready to delete from epoll interst list.\n", close_ev->data.fd);
    int d_res = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, close_ev->data.fd, close_ev);
    if (d_res < 0) {
        perror("epoll_ctl(EPOLL_CTL_DEL)");
        close(close_ev->data.fd);
        return -2;
    }

    return 0;
}

/* defult : UDS connect */
/* if want to select IDS connect -> please '#define IDS_CONNECT' */
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



/* defult : UDS connect */
/* if want to select IDS connect -> please '#define IDS_CONNECT' */
int client_connect_init(sock_info *client) {

#ifdef IDS_CONNECT

    client->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->fd == -1) {
        perror("Socket error");
        return -1;
    } else {
        printf("Socket[IDS] successfully created.\n");
        bzero(&client->addr.ids, sizeof(client->addr.ids));
    }

    client->addr.ids.sin_family = AF_INET;
    client->addr.ids.sin_port = htons(PORT);
    client->addr.ids.sin_addr.s_addr = inet_addr(IP); //in_addr_t inet_addr(const char *cp);
    client->len= sizeof(struct sockaddr_un);

    int result = connect(client->fd, (struct sockaddr *)&client->addr.uds, client->len);
    if (result == -1) {
        perror("Connect error");
        close(client->fd);
        return -2;
    }

#else

    client->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client->fd == -1) {
        perror("Socket error");
        return -1;
    } else {
        printf("Socket[UDS] successfully created.\n");
    }

    client->addr.uds.sun_family = AF_UNIX;
    strcpy(client->addr.uds.sun_path, PATH);
    client->len= sizeof(struct sockaddr_un);

    int result = connect(client->fd, (struct sockaddr *)&client->addr.uds, client->len);
    if (result == -1) {
        perror("Connect error");
        close(client->fd);
        return -2;
    }

#endif

    return 0;
}


