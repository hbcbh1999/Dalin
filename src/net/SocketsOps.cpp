//
// SocketsOps.cpp
//
// Copyright (c) 2017 Jiawei Feng
//

#include "SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

using namespace Dalin;

namespace {

typedef struct sockaddr SA;

const SA* sockaddr_cast(const struct sockaddr_in *addr)
{
    return static_cast<const SA*>(reinterpret_cast<const void*>(addr));
}

SA *sockaddr_cast(struct sockaddr_in *addr)
{
    return static_cast<SA*>(reinterpret_cast<void*>(addr));
}

void setNonblockAndCloseOnExec(int sockfd)
{
    // non-block
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);

    // close-on-exec
    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, 0);
}

}

int SocketsOps::createNonblockingOrDie()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        fprintf(stderr, "Failed in SocketsOps::createNonblockingOrDie()\n");
        abort();
    }

    return sockfd;
}

void SocketsOps::bindOrDie(int sockfd, const struct sockaddr_in &addr)
{
    int ret = ::bind(sockfd, sockaddr_cast(&addr), sizeof(addr));
    if (ret < 0) {
        fprintf(stderr, "Failed in SocketsOps::bindOrDie()\n");
        abort();
    }
}

int SocketsOps::accept(int sockfd, struct sockaddr_in *addr)
{
    socklen_t addrlen = sizeof(*addr);

    int connfd = accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);

    if (connfd < 0) {
        int savedErrno = errno;
        fprintf(stderr, "Failed in SocketsOps::accept() ");
        switch (savedErrno) {
        case EAGAIN:
        case ECONNABORTED:
        case EINTR:
        case EPROTO:
        case EPERM:
        case EMFILE:
            // expected errors
            errno = savedErrno;
            break;
        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
            // unexpected errors
            fprintf(stderr, "unexpected error: %d\n", savedErrno);
            abort();
            break;
        default:
            fprintf(stderr, "unknown error: %d\n", savedErrno);
            abort();
            break;
        }
    }

    return connfd;
}
void SocketsOps::close(int sockfd)
{
    if (::close(sockfd) < 0) {
        fprintf(stderr, "Failed in SocketsOps::close()\n");
    }
}

void SocketsOps::toHostPort(char *buf, size_t size, const struct sockaddr_in &addr)
{
    char host[INET_ADDRSTRLEN] = "INVALID";
    ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof(host));
    uint16_t port = SocketsOps::networkToHost16(addr.sin_port);
    snprintf(buf, size, "%s:%u", host, port);
}

void SocketsOps::fromHostPort(const char *ip, uint16_t port, struct sockaddr_in *addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) < 0) {
        fprintf(stderr, "Failed in SocketsOps::fromHostPort()\n");
    }
}
