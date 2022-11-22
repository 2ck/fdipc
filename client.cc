#include "common.h"

#include <sys/syscall.h>

int main() {
    int ret;
    char buffer[BUFFER_SIZE];
    int recv_fd;

#ifdef USE_PIDFD
    pid_t server_pid;
    int server_fd;
    FILE* pidout = fopen("./pidout.txt", "r");
    fscanf(pidout, "%d,%d", &server_pid, &server_fd);
    int pidfd = syscall(SYS_pidfd_open, server_pid, 0);
    printf("parent pid is %d, pidfd is %d\n", server_pid, pidfd);
    if (pidfd == -1)
        die("pidfd_open");

    recv_fd = syscall(SYS_pidfd_getfd, pidfd, server_fd, 0);
    if (recv_fd == -1)
        die("pidfd_getfd");

#else
    // create local socket
    int data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (data_socket == -1)
        die("socket");

    // connect socket to address
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) -1);
    ret = connect(data_socket, (const struct sockaddr*) &addr, sizeof(addr));
    if (ret == -1)
        die("connect");

    recv_fd = recvfd(data_socket);
    printf("received fd %d over socket\n", recv_fd);

    close(data_socket);
#endif

    print_underlying_filename(recv_fd);

    // read a bit from the file
    FILE* fp = fdopen(recv_fd, "r");
    for (int i = 0; i < 3; i++) {
        char* line = NULL;
        size_t len = 0;
        getline(&line, &len, fp);
        printf("client: %s", line);
    }
    // closes fd too
    fclose(fp);

    return 0;
}
