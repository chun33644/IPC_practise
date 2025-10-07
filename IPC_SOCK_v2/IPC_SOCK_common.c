
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "IPC_SOCK_common.h"
#include "IPC_SOCK_config.h"

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



/* defult : UDS connect */
/* if want select IDS connect -> please '#define IDS_CONNECT' */
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


