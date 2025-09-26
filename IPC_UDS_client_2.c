
#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>

#define  SERVER_PATH       "UDS_Server_socket"
#define  CLI_MAX           3


/*
 *  struct sockaddr_un {i
 *      sa_family_t     sun_family;
 *      char            sun_path[];
 *  };
*/


typedef struct _UDS_info {
    int fd;
    socklen_t len;
    struct sockaddr_un addr;
} UDS_info;

static UDS_info cli_1;


typedef struct _client_info {
    UDS_info *client;
    pthread_t pid;
    char *msg;
} client_info;


static client_info list[CLI_MAX] = {0};

pthread_mutex_t mutex;

pthread_t pid_1;
pthread_t pid_2;
pthread_t pid_3;
pthread_t pid_4;


static char buff[100];



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




void* send_msg_to_server(void *arg) {

    while (1) {

        char *msg = "this msg from client send.";
        ssize_t s_res = send(cli_1.fd, msg, strlen(msg), 0);
        if (s_res < 0) {
            perror("Send error");
            sleep(1);
            continue;
        }
        sleep(1);

    }
    return NULL;
}


void* recv_msg_from_server(void *arg) {

    while(1) {

        lock(__func__);
        memset(buff, 0, sizeof(buff));
        unlock(__func__);

        ssize_t r_res = recv(*(int *)arg, buff, sizeof(buff), 0);
        if (r_res == -1) {
            perror("Recv error");
            break;
        }
        //printf("client[%s] recv msg:%s\n", (char *)arg, buff);

    }
    return NULL;
}


int create_client_UDS(int *cl_fd, struct sockaddr_un *cl_addr, socklen_t *cl_len) {

    *cl_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (*cl_fd == -1) {
        perror("Socket error");
        return -1;
    }

    cl_addr->sun_family = AF_UNIX;
    strcpy(cl_addr->sun_path, SERVER_PATH);
    *cl_len= sizeof(struct sockaddr_un);

    int result = connect(*cl_fd, (struct sockaddr *)cl_addr, *cl_len);
    if (result == -1) {
        perror("Connect error");
        close(*cl_fd);
        return -2;
    }
    return 0;
}

/*

typedef struct _UDS_info {
    int fd;
    socklen_t len;
    struct sockaddr_un addr;
} UDS_info;

static UDS_info cli_1;


typedef struct _client_info {
    UDS_info *client;
    char *msg;
} client_info;


static client_info list[CLI_MAX] = {0};
*/





void* create_multiple_clientfd(void *arg) {

    printf("for test 1.....\n");
    for (int idx = 0; idx < CLI_MAX; idx ++) {
        int res = create_client_UDS(&list[idx].client->fd, &list[idx].client->addr, &list[idx].client->len);
        if (res < 0) {
            printf("idx[%d]create client failed.\n", idx);
            return NULL;
        }
        printf("[%d]fd:%d\n", idx, list[idx].client->fd);
    }

    printf("for test 2.....\n");
    return NULL;
}



/*

        int p_res = pthread_create(&list[idx].pid, NULL, recv_msg_from_server, (int *)&list[idx].client->fd);
        if (p_res != 0) {
            perror("Thread create failed");
        }
*/



int main() {
/*
    int sock_init = create_client_UDS(&cli_1.fd, &cli_1.addr, &cli_1.len);
    if (sock_init < 0){
        printf("Socket init failed.\n");
    }
*/
    //Mutex init
    int m_res = pthread_mutex_init(&mutex, NULL);
    if (m_res != 0) {
        perror("Mutex initialization failed");
    }

    //Thread_1
    int p1_res = pthread_create(&pid_1, NULL, send_msg_to_server, NULL);
    if (p1_res != 0) {
        perror("Thread 1 create failed");
    }
/*
    //Thread_2
    int p2_res = pthread_create(&pid_2, NULL, recv_msg_from_server, NULL);
    if (p2_res != 0) {
        perror("Thread 2 create failed");
    }
*/
    //Thread_3
    int p3_res = pthread_create(&pid_3, NULL, create_multiple_clientfd, NULL);
    if (p3_res != 0) {
        perror("Thread 2 create failed");
    }


    printf("for test......\n");


    printf("Thread readly to join\n");

    pthread_join(pid_1, NULL);
    printf("[pid_1] Join finished\n");

    pthread_join(pid_2, NULL);
    printf("[pid_2] Join finished\n");

    pthread_join(pid_3, NULL);
    printf("[pid_3] Join finished\n");



    close(cli_1.fd);
    return 0;

}
