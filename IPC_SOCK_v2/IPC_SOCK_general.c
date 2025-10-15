#include "IPC_SOCK_config.h"

#include <asm-generic/socket.h>
#include <errno.h>
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
#include <mqueue.h>

#include "IPC_SOCK_general.h"

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
/*
int error_handler(client_info *list) {

    list->start_flag = 0;
    printf("Thread ready to join\n");
    pthread_join(list->rev_pid, NULL);
    pthread_join(list->sen_pid, NULL);

    close(list->s_info.fd);
    memset(list, 0, sizeof(client_info));

    return 0;
}
*/

/* in_use (false) : find the space from lookup table */
/* in_use (true) : find exsiting member from lookup table */
client_info* find_Space_or_Member(int fd, bool in_use, client_info *table) {

    for (int idx = 0; idx < CLI_MAX; idx ++) {

        if (!in_use) {
            //find space
            if (!table[idx].in_use) {
                return &table[idx];
            } else if (table[idx].in_use) {
                continue;
            }
        } else if (in_use) {
            //find member
            if (fd == table[idx].s_info.fd || fd == table[idx].m_info.mq_d) {
                return &table[idx];
            } else {
                continue;
            }
        }
    }
    return NULL;

}

/* release from lookup table */
void release_member(client_info *ptr) {

    if (!ptr) {
        printf("%s() invalid argument !\n", __func__);
        return;
    }


    printf("%s() fd %d ready to release.\n", __func__, ptr->s_info.fd);
    memset(ptr, 0, sizeof(client_info));
}



/* request a message queue qd */
int msgqueue_req(const char *link, int flag, mode_t mode, mqd_t *mq_d, struct mq_attr *attr) {

    attr->mq_maxmsg = 10;
    attr->mq_msgsize = MSG_MAX;

    if (mode) {
        mq_unlink(link);
        *mq_d = mq_open(link, flag, mode, attr);
        if (*mq_d < 0) {
            perror("mq_open");
            return -1;
        }
    } else {
        *mq_d = mq_open(link, flag);
        if (*mq_d < 0) {
            perror("mq_open");
            printf("errno %d : %s\n", errno, strerror(errno));
            return -2;
        }
    }

    return 0;
}


/* register callback function */
void register_callback_func(client_info *ptr, callback cb) {

    ptr->m_info.notify_callback = cb;

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
    printf("epoll  fd %d\n", *new_efd);

    return 0;
}

/* epoll fd and listen fd add to interst list */
int epoll_add(int epoll_fd, int fd, struct epoll_event ev) {

    int c_res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (c_res < 0) {
        perror("epoll_ctl(EPOLL_CTL_ADD)");
        return -1;
    }

    //printf("%s() fd %d added to epoll interst list.\n", __func__, fd);
    return 0;
}


/* delete fd from interst list */
int epoll_delete(int epoll_fd, int fd) {

    if (epoll_fd <= 0 || fd <= 0) {
        printf("invalid argument\n");
        return -1;
    }

    //printf("%s() fd %d ready to delete from epoll interst list.\n", __func__, fd);
    int d_res = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    if (d_res < 0) {
        perror("epoll_ctl(EPOLL_CTL_DEL)");
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
        printf("Socket[IDS] successfully created fd %-2d\n", server->fd);
        bzero(&server->addr.ids, sizeof(server->addr.ids));
    }

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
        printf("Socket[UDS] successfully created fd %-2d\n", server->fd);
    }

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
        printf("Socket[IDS] successfully created fd %-2d\n", client->fd);
        bzero(&client->addr.ids, sizeof(client->addr.ids));
    }

    client->addr.ids.sin_family = AF_INET;
    client->addr.ids.sin_port = htons(PORT);
    client->addr.ids.sin_addr.s_addr = inet_addr(IP); //in_addr_t inet_addr(const char *cp);
    client->len= sizeof(struct sockaddr_un);

    int result = connect(client->fd, (struct sockaddr *)&client->addr.ids, client->len);
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
        printf("Socket[UDS] successfully created fd %-2d\n", client->fd);
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


