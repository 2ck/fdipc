#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>


#define die(fmt) do { perror(fmt); exit(EXIT_FAILURE); } while(0)

#define NUM_FD 1

#define SOCKET_NAME "/tmp/ipctest.socket"
#define BUFFER_SIZE 12

#define USE_PIDFD


/*
 * The following code is taken from man 2 seccomp_unotify
 */

static int
sendfd(int sockfd, int fd)
{
    struct msghdr msgh;
    struct iovec iov;
    int data;
    struct cmsghdr *cmsgp;

    /* Allocate a char array of suitable size to hold the ancillary data.
        However, since this buffer is in reality a 'struct cmsghdr', use a
        union to ensure that it is suitably aligned. */
    union {
        char   buf[CMSG_SPACE(sizeof(int))];
                        /* Space large enough to hold an 'int' */
        struct cmsghdr align;
    } controlMsg;

    /* The 'msg_name' field can be used to specify the address of the
        destination socket when sending a datagram. However, we do not
        need to use this field because 'sockfd' is a connected socket. */

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;

    /* On Linux, we must transmit at least one byte of real data in
        order to send ancillary data. We transmit an arbitrary integer
        whose value is ignored by recvfd(). */

    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    iov.iov_base = &data;
    iov.iov_len = sizeof(int);
    data = 12345;

    /* Set 'msghdr' fields that describe ancillary data */

    msgh.msg_control = controlMsg.buf;
    msgh.msg_controllen = sizeof(controlMsg.buf);

    /* Set up ancillary data describing file descriptor to send */

    cmsgp = CMSG_FIRSTHDR(&msgh);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_RIGHTS;
    cmsgp->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsgp), &fd, sizeof(int));

    /* Send real plus ancillary data */

    if (sendmsg(sockfd, &msgh, 0) == -1)
        return -1;

    return 0;
}

/* Receive a file descriptor on a connected UNIX domain socket. Returns
    the received file descriptor on success, or -1 on error. */

static int
recvfd(int sockfd)
{
    struct msghdr msgh;
    struct iovec iov;
    int data, fd;
    ssize_t nr;

    /* Allocate a char buffer for the ancillary data. See the comments
        in sendfd() */
    union {
        char   buf[CMSG_SPACE(sizeof(int))];
        struct cmsghdr align;
    } controlMsg;
    struct cmsghdr *cmsgp;

    /* The 'msg_name' field can be used to obtain the address of the
        sending socket. However, we do not need this information. */

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;

    /* Specify buffer for receiving real data */

    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    iov.iov_base = &data;       /* Real data is an 'int' */
    iov.iov_len = sizeof(int);

    /* Set 'msghdr' fields that describe ancillary data */

    msgh.msg_control = controlMsg.buf;
    msgh.msg_controllen = sizeof(controlMsg.buf);

    /* Receive real plus ancillary data; real data is ignored */

    nr = recvmsg(sockfd, &msgh, 0);
    if (nr == -1)
        return -1;

    cmsgp = CMSG_FIRSTHDR(&msgh);

    /* Check the validity of the 'cmsghdr' */

    if (cmsgp == NULL ||
            cmsgp->cmsg_len != CMSG_LEN(sizeof(int)) ||
            cmsgp->cmsg_level != SOL_SOCKET ||
            cmsgp->cmsg_type != SCM_RIGHTS) {
        errno = EINVAL;
        return -1;
    }

    /* Return the received file descriptor to our caller */

    memcpy(&fd, CMSG_DATA(cmsgp), sizeof(int));
    return fd;
}

static void print_underlying_filename(int fd)
{
    char procfs_path[PATH_MAX];
    sprintf(procfs_path, "/proc/self/fd/%d", fd);
    char real_path[PATH_MAX];
    /* readlink does not null-terminate */
    int nbytes = readlink(procfs_path, real_path, 1000);
    if (nbytes == -1)
        perror("readlink");
    else
        printf("fd %d resolves to %.*s\n", fd, nbytes, real_path);
}
