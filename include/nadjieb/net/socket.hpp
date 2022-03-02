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

static bool initSocket() {
    signal(SIGPIPE, SIG_IGN);
    return true;
}

static void closeSocket(SocketFD sockfd) {
    ::close(sockfd);
}

static void panicIfUnexpected(bool condition, const std::string& message, const SocketFD sockfd) {
    if (condition) {
        closeSocket(sockfd);
        throw std::runtime_error(message);
    }
}

static SocketFD createSocket(int af, int type, int protocol) {
    SocketFD sockfd = ::socket(af, type, protocol);

    panicIfUnexpected(sockfd == SOCKET_ERROR, "createSocket() failed", sockfd);

    return sockfd;
}

static void setSocketReuseAddress(SocketFD sockfd) {
    const int enable = 1;
    auto res = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));

    panicIfUnexpected(res == SOCKET_ERROR, "setSocketReuseAddress() failed", sockfd);
}

static void setSocketReusePort(SocketFD sockfd) {
    const int enable = 1;
    auto res = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

    panicIfUnexpected(res == SOCKET_ERROR, "setSocketReusePort() failed", sockfd);
}

static void setSocketNonblock(SocketFD sockfd) {
    unsigned long ul = true;
    auto res = ioctl(sockfd, FIONBIO, &ul);

    panicIfUnexpected(res == SOCKET_ERROR, "setSocketNonblock() failed", sockfd);
}

static void bindSocket(SocketFD sockfd, const char* ip, int port) {
    struct sockaddr_in ip_addr;
    ip_addr.sin_family = AF_INET;
    ip_addr.sin_port = htons(port);
    ip_addr.sin_addr.s_addr = INADDR_ANY;
    auto res = ::inet_pton(AF_INET, ip, &ip_addr.sin_addr);
    panicIfUnexpected(res <= 0, "inet_pton() failed", sockfd);

    res = ::bind(sockfd, (struct sockaddr*)&ip_addr, sizeof(ip_addr));
    panicIfUnexpected(res == SOCKET_ERROR, "bindSocket() failed", sockfd);
}

static void listenOnSocket(SocketFD sockfd, int backlog) {
    auto res = ::listen(sockfd, backlog);
    panicIfUnexpected(res == SOCKET_ERROR, "listenOnSocket() failed", sockfd);
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
