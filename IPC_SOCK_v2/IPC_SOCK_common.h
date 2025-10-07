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



#define  MSG_MAX           100


typedef union _sock_type {
    struct sockaddr_in ids;
    struct sockaddr_un uds;
} sock_type;


typedef struct _sock_info {
    int fd;
    socklen_t len;
    union _sock_type addr;
} sock_info;


typedef enum _error_code {
    ERROR_EXCEEDED_CONEECTIONS_NUM = 1,


} error_code;

typedef struct _package {
    int header;
    char msg[MSG_MAX];
    error_code err;
} package;


typedef struct _client_info {
    sock_info client;
    package pkg;
    pthread_t r_pid;
    pthread_t s_pid;
    bool in_use;
    int start_flag;
} client_info;


/* mutex lock */
int lock(pthread_mutex_t *mutex, const char *func_name);

/* mutex unlock */
int unlock(pthread_mutex_t *mutex, const char *func_name);

/* join thread & close fd & memset info */
int error_handler(client_info *list);


#endif
