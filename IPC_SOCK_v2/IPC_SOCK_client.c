#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "IPC_SOCK_common.h"
#include "IPC_SOCK_server.h"



/* select connection type */
#define IDS_CONNECT

#define  CLI_MAX           6

static client_info list[CLI_MAX] = {0};


static pthread_mutex_t mutex;

static pthread_t pid_1;


void* send_msg_to_server(void *arg) {


    while (1) {

        client_info *ptr = (client_info *)arg;

        snprintf(ptr->pkg.msg, 100, "this msg from client %d send.", ptr->client.fd);

        if (!ptr->start_flag) {
            break;
        } else if (ptr->start_flag) {
            ssize_t s_res = send(ptr->client.fd, &ptr->pkg, sizeof(package), 0);
            if (s_res == -1) {
                perror("Send error");
                return NULL;
            }

            memset(ptr->pkg.msg, 0, sizeof(package));
            sleep(2);
            // printf("ready to close .... fd %d", ptr->client.fd);
            //error_handler(ptr);
        }
    }
    return NULL;
}


void* recv_msg_from_server(void *arg) {

    while(1) {

        client_info *ptr = (client_info *)arg;
        if (!ptr->start_flag) {
            break;
        } else if (ptr->start_flag) {

            ssize_t r_res = recv(ptr->client.fd, &ptr->pkg, sizeof(package), 0);
            if (r_res < 0) {
                perror("Recv error");
                continue;
            }

            printf("%s\n", ptr->pkg.msg);
        }
    }
    return NULL;
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



void* create_multiple_clientfd(void *arg) {

    client_info *ptr = (client_info *)arg;

    for (int idx = 0; idx < CLI_MAX; idx ++) {
        int res = client_connect_init(&ptr[idx].client);
        if (res < 0) {
            printf("idx[%d]create client failed.\n", idx);
            return NULL;
        }
        printf("[%d]fd:%d\n", idx, ptr[idx].client.fd);

        ptr[idx].start_flag = 1;
        int s_res = pthread_create(&(ptr[idx].s_pid), NULL, send_msg_to_server, (client_info *)&ptr[idx]);
        int r_res = pthread_create(&(ptr[idx].r_pid), NULL, recv_msg_from_server, (client_info *)&ptr[idx]);
        if (s_res != 0 || r_res != 0) {
            printf("list[%d] recv or send thread create failed....\n", idx);
            error_handler(&ptr[idx]);
        }
        ptr[idx].in_use = true;
    }

    for (int idx = 0; idx < CLI_MAX; idx ++) {
        pthread_join(ptr[idx].s_pid, NULL);
        pthread_join(ptr[idx].r_pid, NULL);
    }

    return NULL;
}

int main() {

    //Mutex init
    int m_res = pthread_mutex_init(&mutex, NULL);
    if (m_res != 0) {
        perror("Mutex initialization failed");
    }

    //Thread_1
    int p1_res = pthread_create(&pid_1, NULL, create_multiple_clientfd, (client_info *)list);
    if (p1_res != 0) {
        perror("Thread 2 create failed");
    }

    pthread_join(pid_1, NULL);

    //close

    return 0;

}
