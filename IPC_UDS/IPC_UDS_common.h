#ifndef IPC_UDS_COMMON_H
#define IPC_UDS_COMMON_H

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define  MSG_MAX           100

/*
 *  struct sockaddr_un {
 *      sa_family_t     sun_family;
 *      char            sun_path[];
 *  };
*/


typedef struct _UDS_info {
    int fd;
    socklen_t len;
    struct sockaddr_un addr;
} UDS_info;


typedef struct _client_info {
    UDS_info client;
    pthread_t r_pid;
    pthread_t s_pid;
    char msg[MSG_MAX];
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
