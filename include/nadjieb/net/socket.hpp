#pragma once

#include <nadjieb/utils/platform.hpp>

#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <WinError.h>
#include <Ws2tcpip.h>
#include <errno.h>
#include <winsock.h>
#include <winsock2.h>
#elif defined NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX
#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <stdexcept>

namespace nadjieb {
namespace net {

typedef int SocketFD;

const int SOCKET_ERROR = -1;

static bool initSocket() {
    bool ret = true;
#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
    static WSADATA g_WSAData;
    static bool WinSockIsInit = false;
    if (WinSockIsInit) {
        return true;
    }
    if (WSAStartup(MAKEWORD(2, 2), &g_WSAData) == 0) {
        WinSockIsInit = true;
    } else {
        ret = false;
    }
#elif defined NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX || defined NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
    signal(SIGPIPE, SIG_IGN);
#endif
    return true;
}

static void destroySocket() {
#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
    WSACleanup();
#endif
}

static void closeSocket(SocketFD sockfd) {
#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
    ::closesocket(sockfd);
#else
    ::close(sockfd);
#endif
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
#if defined NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX || defined NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
    const int enable = 1;
    auto res = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

    panicIfUnexpected(res == SOCKET_ERROR, "setSocketReusePort() failed", sockfd);
#endif
}

static void setSocketNonblock(SocketFD sockfd) {
    unsigned long ul = true;
    int res;
#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
    res = ioctlsocket(sockfd, FIONBIO, &ul);
#else
    res = ioctl(sockfd, FIONBIO, &ul);
#endif
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

static int pollSockets(struct pollfd* fds, int nfds, int timeout) {
#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
    return WSAPoll(&fds[0], nfds, timeout);
#elif defined NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX || defined NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
    return poll(fds, nfds, timeout);
#endif
}
}  // namespace net
}  // namespace nadjieb
