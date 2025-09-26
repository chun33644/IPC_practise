
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define  SERVER_PATH       "UDS_Server_socket"
#define  MSG_MAX           100
#define  CLI_MAX           5


/*
 *  struct sockaddr_un {
 *      sa_family_t     sun_family;
 *      char            sun_path[];
 *  };
*/



// can use include.h
typedef struct _UDS_info {
    int fd;
    socklen_t len;
    struct sockaddr_un addr;
} UDS_info;


static UDS_info server = {0};


typedef struct _client_info {
    int cfd;
    socklen_t clen;
    struct sockaddr_un caddr;
    char msg[MSG_MAX];
} client_info;


static client_info client_interface = {0};

/*
typedef struct _client_list {
    client_info *info;
    bool connect_stat;
    bool in_use;
} client_list;


static client_list cli_list[CLI_MAX];
*/


pthread_t pid_1, pid_2, pid_3;

static int recv_exit_flag = 1;
static int send_exit_flag = 1;
static int stopAll_flag = 1;

static int start_flag = 0;

static char temp_buff[MSG_MAX];

static pthread_mutex_t mutex;



int lock(const char *func_name) {

    int res = pthread_mutex_lock(&mutex);
    if (res != 0) {
        printf("lock error\n");
    }

    //printf("%s lock\n", func_name);
    return 0;
}


int unlock(const char *func_name) {
    int res = pthread_mutex_unlock(&mutex);
    if (res != 0) {
        printf("unlock error\n");
    }

    //printf("%s unlock\n", func_name);
    return 0;
}


/*
int monitor_connect(int *se_fd, int *cl_fd, struct sockaddr_un *cl_addr, socklen_t *cl_len) {

    *cl_len = sizeof(socklen_t);
    *cl_fd = accept(*se_fd, (struct sockaddr *)cl_addr, cl_len);
    if (*cl_fd == -1) {
        perror("Accept error");
        recv_exit_flag = 0;
        send_exit_flag = 0;
        close(*cl_fd);
        return -1;
    }
    printf("client fd: %d\n", client_interface.cfd);
    return 0;
}
*/




/* if accept() return error, it will stop all threads. */
void* monitor_connect_task(void *arg) {

    while (stopAll_flag) {

        client_interface.cfd = accept(*(int *)arg, NULL, NULL);
        if (client_interface.cfd < 0) {
            perror("Accept error");
            continue;
        } else if (client_interface.cfd > 0) {
            start_flag = 1;   //start recv and send
        }
    }
    return 0;

}

void* recv_msg_from_client(void *arg) {

    while (recv_exit_flag) {

        if (!start_flag) {
            printf("wait client connect......\n");
            sleep(1);
            continue;
        } else if (start_flag) {
            char r_buff[100] = {0};

            ssize_t r_bytes = recv(client_interface.cfd, r_buff, sizeof(r_buff), 0);
            if (r_bytes == -1) {
                perror("Recv error");
                break;
            } else if (r_bytes > 0) {
                lock(__func__);
                strcpy(temp_buff, r_buff);
                memset(r_buff, 0, sizeof(r_buff));
                unlock(__func__);
            }
        }
    }
    return NULL;
}



void* send_msg_to_client(void *arg) {

    while (send_exit_flag) {

        if (!start_flag) {
            sleep(1);
            continue;
        } else if (start_flag) {
            sleep(1);
            lock(__func__);
            char s_buff[100] = {0};
            strcpy(s_buff, temp_buff);
            unlock(__func__);

            int byte = strlen(s_buff);
            if (s_buff[byte] == '\0') {
                ssize_t s_bytes = send(client_interface.cfd, s_buff, strlen(s_buff), 0);
                if (s_bytes == -1) {
                    perror("Send error");
                    break;
                }
            }
        }
    }
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

    //Thread_3
    int p3_res = pthread_create(&pid_3, NULL, monitor_connect_task, (int *)&server.fd);
    if (p3_res != 0) {
        perror("monitor_connect_task thread create failed");
    }

    //Thread_1
    int p1_res = pthread_create(&pid_1, NULL, recv_msg_from_client, NULL);
    if (p1_res != 0) {
        perror("recv_msg_from_client thread create failed");
    }

    //Thread_2
    int p2_res = pthread_create(&pid_2, NULL, send_msg_to_client, NULL);
    if (p2_res != 0) {
        perror("send_msg_to_client thread create failed");
    }


    printf("Thread readly to join\n");

    pthread_join(pid_1, NULL);
    printf("[pid_1] Join finished\n");

    pthread_join(pid_2, NULL);
    printf("[pid_2] Join finished\n");

    pthread_join(pid_3, NULL);
    printf("[pid_3] Join finished\n");


    close(client_interface.cfd);


    return 0;

}
