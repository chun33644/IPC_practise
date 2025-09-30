
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "IPC_UDS_common.h"
#include "IPC_UDS_server.h"


static UDS_info server = {0};

static client_info client_info_list[CLI_MAX] = {0};

static pthread_t pid_1;
static pthread_t pid_2;

static int stopAll_flag = 1;

static char temp_buff[MSG_MAX];

static pthread_mutex_t mutex;

void* recv_msg_from_client(void *arg) {

    while (stopAll_flag) {

        client_info *ptr = (client_info *)arg;
        if (!ptr->start_flag) {
            break;
        } else if (ptr->start_flag) {
            char r_buff[100] = {0};

            ssize_t r_bytes = recv(ptr->client.fd, r_buff, sizeof(r_buff), 0);
            if (r_bytes == -1) {
                perror("Recv error");
                break;
            } else if (r_bytes > 0) {
                lock(&mutex, __func__);
                strcpy(ptr->msg, r_buff);
                memset(r_buff, 0, sizeof(r_buff));
                unlock(&mutex, __func__);
                printf("fd %d mag:%s\n", ptr->client.fd, ptr->msg);
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
        char s_buff[100] = {0};

        if (!ptr->start_flag) {
            break;
        } else if (ptr->start_flag) {
            sleep(1);
            lock(&mutex, __func__);
            strcpy(s_buff, ptr->msg);
            memset(ptr->msg, 0, sizeof(ptr->msg));
            unlock(&mutex, __func__);

            //int byte = strlen(s_buff);
            //if (s_buff[byte] == '\0') {
                ssize_t s_bytes = send(ptr->client.fd, s_buff, strlen(s_buff), 0);
                if (s_bytes == -1) {
                    perror("Send error");
                    break;
                }
                //printf("fd %d (%s): %s\n", ptr->client.fd, __func__ , s_buff);
            //}
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
            continue;
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


int create_server_UDS(int *se_fd, struct sockaddr_un *se_addr, socklen_t *se_len) {

    *se_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (*se_fd == -1) {
        perror("Socket error");
        return -1;
    }

    unlink(SERVER_PATH);
    se_addr->sun_family = AF_UNIX;
    strcpy(se_addr->sun_path, SERVER_PATH);
    *se_len = sizeof(struct sockaddr_un);

    int bind_res = bind(*se_fd, (struct sockaddr *)se_addr, *se_len);
    if (bind_res == -1) {
        perror("Bind error");
        return -2;
    }

    printf("server path: %s\n", se_addr->sun_path);

    int listen_res = listen(*se_fd, 5);
    if (listen_res == -1) {
        perror("Listen error");
        return -3;
    }
    return 0;
}


int main() {


    int sock_init = create_server_UDS(&server.fd, &server.addr, &server.len);
    if (sock_init < 0) {
        printf("Socket init failed.\n");
    }

    //Mutex
    int m_res = pthread_mutex_init(&mutex, NULL);
    if (m_res != 0) {
        perror("Mutex initialization failed");
    }

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

    printf("%s\n", __func__);
    return 0;

}
