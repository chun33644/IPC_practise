
#include <stdio.h>
#include <string.h>
#include <pthread.h>

int counter = 0;
pthread_mutex_t mutex;


void* thread_func(void *arg) {

    for (int idx = 0; idx < 100000; idx++) {
        pthread_mutex_lock(&mutex);
        counter ++;
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}


void* print_thread_func(void *arg) {

    char *message = (char *)arg;
    printf("newthread: received ---> %s\n", message);
    return NULL;
}


int main() {

//test counter final value.....
    pthread_t t1, t2;
    //init mutex
    int init_res = pthread_mutex_init(&mutex, NULL);
    if (init_res != 0) {
        printf("Pthread init fail: %s\n", strerror(init_res));
    }


    //create two thread
    int new1_res = pthread_create(&t1, NULL, thread_func, NULL);
    if (new1_res != 0) {
        printf("Pthread create fail: %s\n", strerror(new1_res));
    }
    printf("t1 PID: %lu\n", (unsigned long)t1);

    int new2_res = pthread_create(&t2, NULL, thread_func, NULL);
    if (new2_res != 0) {
        printf("Pthread create fail: %s\n", strerror(new1_res));
    }
    printf("t2 PID: %lu\n", (unsigned long)t2);

    //wait two thread until end
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("check counter value: %d\n", counter);

    pthread_mutex_destroy(&mutex);

    printf("---------------------------------------\n");
//test thread whether entry print_thread_func
    pthread_t t3;
    char *message = "Hello.....";

    //create
    int new3_res = pthread_create(&t3, NULL, print_thread_func, (void *)message);
    if (new3_res != 0) {
        printf("Pthread create fail: %s\n", strerror(new3_res));
    }
    printf("t3 PID: %lu\n", (unsigned long)t3);

    pthread_join(t3, NULL);

    printf("t3 end---------------------------------\n");


    return 0;

}
