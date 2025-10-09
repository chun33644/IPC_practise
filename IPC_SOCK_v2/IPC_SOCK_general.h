#ifndef IPC_UDS_COMMON_H
#define IPC_UDS_COMMON_H

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <mqueue.h>

#include "IPC_SOCK_config.h"

/*
typedef enum _error_code {
    ERROR_EXCEEDED_CONNECTIONS_NUM = 1,


} error_code;
*/

typedef union _sock_type {
    struct sockaddr_in ids;
    struct sockaddr_un uds;
} sock_type;


typedef struct _sock_info {
    int fd;
    socklen_t len;
    union _sock_type addr;

    //for epoll_monitor
    struct epoll_event ev;

} sock_info;


typedef struct _package {
    int header;
    char msg[MSG_MAX];
    int payload_len;
    //error_code err;
} package;


// for notify something of status
typedef void (*callback)(mqd_t mq_d, package *pkg);

typedef struct _msqueue_info{
    mqd_t mq_d;
    int flag;
    struct mq_attr mq_att;
    callback notify_callback;
} msgqueue_info;


typedef struct _client_info {
    sock_info s_info;
    package pkg;
    msgqueue_info m_info;
    pthread_t rev_pid;
    pthread_t sen_pid;
    bool in_use;
    int start_flag;
} client_info;


/* mutex lock */
int lock(pthread_mutex_t *mutex, const char *func_name);

/* mutex unlock */
int unlock(pthread_mutex_t *mutex, const char *func_name);



/* join thread & close fd & memset info */
int error_handler(client_info *list);


/* in_use (false) : find the space from lookup table */
/* in_use (true) : find exsiting member from lookup table */
client_info* find_Space_or_Member(int fd, bool in_use, client_info *table);


/* release from lookup table */
void release_member(int fd, client_info *table);


/* request a message queue qd */
int msgqueue_req(const char *link, int flag, mode_t mode, msgqueue_info *info);



/* register callback function */
void register_callback_func(client_info *ptr, callback cb);




/* setting fd flag (O_NONBLOCK) */
int set_nonblocking(int fd);

/* register epoll fd */
int epoll_req(int *new_efd);

/* epoll fd and listen fd add to interst list */
int epoll_add(int epoll_fd, sock_info *info);

/* delete fd from interst list */
int epoll_delete(int epoll_fd, struct epoll_event *close_ev);



/* defult : UDS connect */
/* if want select IDS connect -> please '#define IDS_CONNECT' */
int server_connect_init(sock_info *server);
int client_connect_init(sock_info *client);




#endif
