
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "IPC_UDS_common.h"

int lock(pthread_mutex_t *mutex, const char *func_name) {

    int res = pthread_mutex_lock(mutex);
    if (res != 0) {
        printf("lock error (%s)\n", func_name);
    }

    //printf("%s lock\n", func_name);
    return 0;
}

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
    pthread_join(list->r_pid, NULL);
    pthread_join(list->s_pid, NULL);

    close(list->client.fd);
    memset(list, 0, sizeof(client_info));

    return 0;
}
