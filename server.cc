#include <poll.h>


#include "common.h"

int main(int argc, char* argv[]) {
    int ret;
    char buffer[BUFFER_SIZE];
    const char* file_name = "./testfile.txt";
    int send_fd;

    if (argc > 1)
        file_name = argv[1];

    send_fd = open(file_name, O_RDONLY);

    // read a bit from the file
    FILE* fp = fdopen(send_fd, "r");
    for (int i = 0; i < 3; i++) {
        char* line = NULL;
        size_t len = 0;
        getline(&line, &len, fp);
        printf("server: %s", line);
    }
    // do not fclose because that closes the fd too

#ifdef USE_PIDFD
    // write own pid and our fd to file
    FILE* pidout = fopen("./pidout.txt", "w");
    fprintf(pidout, "%d,%d", getpid(), send_fd);
    fclose(pidout);

    while(1){ usleep(1e5); };
#else
    // create local socket
    int connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connection_socket == -1)
        die("socket");

    // bind the socket to a name
    struct sockaddr_un name = {0};
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) -1);
    ret = bind(connection_socket, (const struct sockaddr*) &name, sizeof(name));
    if (ret == -1)
        die("bind");

    // prepare for accepting connections, backlog size = 20
    ret = listen(connection_socket, 20);
    if (ret == -1)
        die("listen");

    int data_socket = accept(connection_socket, NULL, NULL);
    if (data_socket == -1)
        die("accept");

    sendfd(data_socket, send_fd);

    close(data_socket);

    close(connection_socket);
    unlink(SOCKET_NAME);
#endif

    return 0;
}
