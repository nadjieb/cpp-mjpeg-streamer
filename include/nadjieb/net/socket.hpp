#pragma once

#include <nadjieb/utils/platform.hpp>

#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
#define _WIN32_WINNT 0x600
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#include <WinError.h>
#include <errno.h>
#elif defined NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#error "Unsupported OS, please commit an issue."
#endif

#include <stdexcept>

namespace nadjieb {
namespace net {

#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
typedef SOCKET SocketFD;
#define NADJIEB_MJPEG_STREAMER_POLLFD WSAPOLLFD
#define NADJIEB_MJPEG_STREAMER_ERRNO WSAGetLastError()
#define NADJIEB_MJPEG_STREAMER_ENOTSOCK WSAENOTSOCK
#define NADJIEB_MJPEG_STREAMER_EWOULDBLOCK WSAEWOULDBLOCK
#define NADJIEB_MJPEG_STREAMER_EINTR WSAEINTR
#define NADJIEB_MJPEG_STREAMER_ECONNABORTED WSAECONNABORTED
#define NADJIEB_MJPEG_STREAMER_SOCKET_ERROR SOCKET_ERROR
#define NADJIEB_MJPEG_STREAMER_INVALID_SOCKET INVALID_SOCKET

#elif defined NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX || defined NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
#define NADJIEB_MJPEG_STREAMER_POLLFD pollfd
#define NADJIEB_MJPEG_STREAMER_ERRNO errno
#define NADJIEB_MJPEG_STREAMER_ENOTSOCK EBADF
#define NADJIEB_MJPEG_STREAMER_EWOULDBLOCK EAGAIN
#define NADJIEB_MJPEG_STREAMER_EINTR EINTR
#define NADJIEB_MJPEG_STREAMER_ECONNABORTED ECONNABORTED
typedef int SocketFD;
#define NADJIEB_MJPEG_STREAMER_SOCKET_ERROR (-1)
#define NADJIEB_MJPEG_STREAMER_INVALID_SOCKET (-1)
#endif

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

    panicIfUnexpected(sockfd == NADJIEB_MJPEG_STREAMER_SOCKET_ERROR, "createSocket() failed", sockfd);

    return sockfd;
}

static void setSocketReuseAddress(SocketFD sockfd) {
    const int enable = 1;
    auto res = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));

    panicIfUnexpected(res == NADJIEB_MJPEG_STREAMER_SOCKET_ERROR, "setSocketReuseAddress() failed", sockfd);
}

static void setSocketReusePort(SocketFD sockfd) {
#if defined NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX || defined NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
    const int enable = 1;
    auto res = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

    panicIfUnexpected(res == NADJIEB_MJPEG_STREAMER_SOCKET_ERROR, "setSocketReusePort() failed", sockfd);
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
    panicIfUnexpected(res == NADJIEB_MJPEG_STREAMER_SOCKET_ERROR, "setSocketNonblock() failed", sockfd);
}

static void bindSocket(SocketFD sockfd, const char* ip, int port) {
    struct sockaddr_in ip_addr;
    ip_addr.sin_family = AF_INET;
    ip_addr.sin_port = htons(port);
    ip_addr.sin_addr.s_addr = INADDR_ANY;
    auto res = inet_pton(AF_INET, ip, &ip_addr.sin_addr);
    panicIfUnexpected(res <= 0, "inet_pton() failed", sockfd);

    res = ::bind(sockfd, (struct sockaddr*)&ip_addr, sizeof(ip_addr));
    panicIfUnexpected(res == NADJIEB_MJPEG_STREAMER_SOCKET_ERROR, "bindSocket() failed", sockfd);
}

static void listenOnSocket(SocketFD sockfd, int backlog) {
    auto res = ::listen(sockfd, backlog);
    panicIfUnexpected(res == NADJIEB_MJPEG_STREAMER_SOCKET_ERROR, "listenOnSocket() failed", sockfd);
}

static SocketFD acceptNewSocket(SocketFD sockfd) {
    return ::accept(sockfd, nullptr, nullptr);
}

static int readFromSocket(int socket, char* buffer, size_t length, int flags) {
    return ::recv(socket, buffer, length, flags);
}

static int sendViaSocket(int socket, const char* buffer, size_t length, int flags) {
    return ::send(socket, buffer, length, flags);
}

static int pollSockets(NADJIEB_MJPEG_STREAMER_POLLFD* fds, int nfds, long timeout) {
#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
    return WSAPoll(&fds[0], nfds, timeout);
#elif defined NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX || defined NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
    return poll(fds, nfds, timeout);
#endif
}
}  // namespace net
}  // namespace nadjieb
