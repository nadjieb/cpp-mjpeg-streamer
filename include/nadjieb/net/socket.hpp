#pragma once

#include <arpa/inet.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdexcept>

namespace nadjieb {
namespace net {

typedef int SocketFD;

const int SOCKET_ERROR = -1;
const SocketFD SOCKET_INVALID = -1;

static bool initSocket() {
    signal(SIGPIPE, SIG_IGN);
    return true;
}

static SocketFD createSocket(int af, int type, int protocol) {
    SocketFD sockfd = ::socket(af, type, protocol);

    if (sockfd == SOCKET_ERROR) {
        throw std::runtime_error("createSocket() failed");
    }

    return sockfd;
}

static void closeSocket(SocketFD sockfd) {
    ::close(sockfd);
}

static void setSocketReuseAddress(SocketFD sockfd) {
    const int enable = 1;
    auto res = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));

    if (res == SOCKET_ERROR) {
        closeSocket(sockfd);
        throw std::runtime_error("setSocketReuseAddress() failed");
    }
}

static void setSocketReusePort(SocketFD sockfd) {
    const int enable = 1;
    auto res = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

    if (res == SOCKET_ERROR) {
        closeSocket(sockfd);
        throw std::runtime_error("setSocketReusePort() failed");
    }
}

static void setSocketNonblock(SocketFD sockfd) {
    unsigned long ul = true;
    int res = ioctl(sockfd, FIONBIO, &ul);

    if (res == SOCKET_ERROR) {
        closeSocket(sockfd);
        throw std::runtime_error("setSocketNonblock() failed");
    }
}

static void bindSocket(SocketFD sockfd, const char* ip, int port) {
    struct sockaddr_in ip_addr;
    ip_addr.sin_family = AF_INET;
    ip_addr.sin_port = htons(port);
    ip_addr.sin_addr.s_addr = INADDR_ANY;
    auto res = inet_pton(AF_INET, ip, &ip_addr.sin_addr);
    if (res <= 0) {
        closeSocket(sockfd);
        throw std::runtime_error("inet_pton() failed");
    }

    res = ::bind(sockfd, (struct sockaddr*)&ip_addr, sizeof(ip_addr));
    if (res == SOCKET_ERROR) {
        closeSocket(sockfd);
        throw std::runtime_error("bindSocket() failed");
    }
}

static void listenOnSocket(SocketFD sockfd, int backlog) {
    auto res = ::listen(sockfd, backlog);
    if (res == SOCKET_ERROR) {
        closeSocket(sockfd);
        throw std::runtime_error("listenOnSocket() failed");
    }
}

static SocketFD acceptNewSocket(SocketFD sockfd) {
    return ::accept(sockfd, nullptr, nullptr);
}

static int readFromSocket(int socket, void* buffer, size_t length, int flags) {
    return ::recv(socket, buffer, length, flags);
}

static int sendViaSocket(int socket, const void* buffer, size_t length, int flags) {
    return ::send(socket, buffer, length, flags);
}
}  // namespace net
}  // namespace nadjieb
