


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>



void my_signal_handler(int sig) {

    printf("hihi~~~welcome to my signal handler\n");

}


void run_parent_process(int write_f) {

    const char *message = "from parent process!";
    printf("parent process is writing msg to the pip.\n");

    ssize_t w_result = write(write_f, message, strlen(message)+1);
    if (w_result < 0 || w_result == 0) {
        perror("write error:");
        exit(EXIT_FAILURE);
    } else {
        //printf("write success! %zd\n", w_result);
    }

    int c_result = close(write_f);
    if (c_result < 0){
        perror("close error:");
        exit(EXIT_FAILURE);
    } else {
        //printf("close success! %d\n", c_result);
    }

    printf("parent process finished writing and is waiting for child.\n");
    wait(NULL);

}


void run_child_process(int read_f) {

    char buffer[100];
    printf("child process is reading from the pipe....\n");

    ssize_t r_result = read(read_f, buffer, sizeof(buffer));
    if (r_result < 0 || r_result == 0) {
        perror("read error:");
        exit(EXIT_FAILURE);
    } else {
        //printf("read success! %zd\n", r_result);
    }

    int c_result = close(read_f);
    if (c_result < 0){
        perror("close error:");
        exit(EXIT_FAILURE);
    } else {
        //printf("close success! %d\n", c_result);
    }

    printf("child process received message: %s\n", buffer);
    exit(EXIT_SUCCESS);

}


int main() {

    struct sigaction sa;
    //sigacion init set
    sa.sa_handler = my_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    //register signal process
    int sig_result = sigaction(SIGINT, &sa, NULL);
    if (sig_result < 0) {
        perror("sigaction error: ");
        exit(EXIT_FAILURE);
    }

    int pipefd[2];    //pipefd[0] for read, pipefd[1] for write
    pid_t pid;

    //create a pipe
    if (pipe(pipefd) < 0) {
        perror("pipe:");
        exit(EXIT_FAILURE);
    }

    //create a child process
    pid = fork();
    if (pid < 0) {
    perror("fork:");
        exit(EXIT_FAILURE);
    }

    //use fork() to identify the parent process or child process
    if (pid > 0) {
        printf("enter the parent process, ID: %d\n", pid);
        close(pipefd[0]);
        run_parent_process(pipefd[1]);
    } else {
        printf("enter the child process, ID: %d\n", pid);
        close(pipefd[1]);
        run_child_process(pipefd[0]);
    }

    //check file decriptor number
    //at POSIX standard, have stdin(0), stdout(1), stderr(2) have exists
    for (int idx = 0; idx < sizeof(pipefd)/sizeof(pipefd[0]); idx ++) {
        printf("pipefd[%d]: %d\n", idx, pipefd[idx]);
    }

    //pause() execution until a signal is caught
    while(1) {
        pause();
    }


    return 0;

}


