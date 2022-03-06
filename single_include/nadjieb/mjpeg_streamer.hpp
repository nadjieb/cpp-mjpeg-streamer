/*
C++ MJPEG over HTTP Library
https://github.com/nadjieb/cpp-mjpeg-streamer

MIT License

Copyright (c) 2020-2022 Muhammad Kamal Nadjieb

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

// #include <nadjieb/utils/version.hpp>


/// The major version number
#define NADJIEB_MJPEG_STREAMER_VERSION_MAJOR 3

/// The minor version number
#define NADJIEB_MJPEG_STREAMER_VERSION_MINOR 0

/// The patch number
#define NADJIEB_MJPEG_STREAMER_VERSION_PATCH 0

/// The complete version number
#define NADJIEB_MJPEG_STREAMER_VERSION_CODE (NADJIEB_MJPEG_STREAMER_VERSION_MAJOR * 10000 + NADJIEB_MJPEG_STREAMER_VERSION_MINOR * 100 + NADJIEB_MJPEG_STREAMER_VERSION_PATCH)

/// Version number as string
#define NADJIEB_MJPEG_STREAMER_VERSION_STRING "3.0.0"


// #include <nadjieb/net/http_request.hpp>


#include <sstream>
#include <string>
#include <unordered_map>

// Reference https://developer.mozilla.org/en-US/docs/Web/HTTP/Messages#http_requests

namespace nadjieb {
namespace net {
class HTTPRequest {
   public:
    HTTPRequest(const std::string& message) { parse(message); }

    void parse(const std::string& message) {
        std::istringstream iss(message);

        std::getline(iss, method_, ' ');
        std::getline(iss, target_, ' ');
        std::getline(iss, version_, '\r');

        std::string line;
        std::getline(iss, line);

        while (true) {
            std::getline(iss, line);
            if (line == "\r") {
                break;
            }

            std::string key;
            std::string value;
            std::istringstream iss_header(line);
            std::getline(iss_header, key, ':');
            std::getline(iss_header, value, ' ');
            std::getline(iss_header, value, '\r');

            headers_[key] = value;
        }

        body_ = iss.str().substr(iss.tellg());
    }

    const std::string& getMethod() const { return method_; }

    const std::string& getTarget() const { return target_; }

    const std::string& getVersion() const { return version_; }

    const std::string& getValue(const std::string& key) { return headers_[key]; }

    const std::string& getBody() const { return body_; }

   private:
    std::string method_;
    std::string target_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};
}  // namespace net
}  // namespace nadjieb

// #include <nadjieb/net/http_response.hpp>


#include <sstream>
#include <string>
#include <unordered_map>

// Reference https://developer.mozilla.org/en-US/docs/Web/HTTP/Messages#http_responses

namespace nadjieb {
namespace net {
class HTTPResponse {
   public:
    std::string serialize() {
        const std::string delimiter = "\r\n";
        std::stringstream stream;

        stream << version_ << ' ' << status_code_ << ' ' << status_text_ << delimiter;

        for (const auto& header : headers_) {
            stream << header.first << ": " << header.second << delimiter;
        }

        stream << delimiter << body_;

        return stream.str();
    }

    void setVersion(const std::string& version) { version_ = version; }
    void setStatusCode(const int& status_code) { status_code_ = status_code; }
    void setStatusText(const std::string& status_text) { status_text_ = status_text; }
    void setValue(const std::string& key, const std::string& value) { headers_[key] = value; }
    void setBody(const std::string& body) { body_ = body; }

   private:
    std::string version_;
    int status_code_;
    std::string status_text_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};
}  // namespace net
}  // namespace nadjieb

// #include <nadjieb/net/listener.hpp>


// #include <nadjieb/net/socket.hpp>


// #include <nadjieb/utils/platform.hpp>


#if defined _MSC_VER || defined __MINGW32__
#define NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
#elif defined __APPLE_CC__ || defined __APPLE__
#define NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
#else
#define NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX
#endif


#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
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
#include <string>

namespace nadjieb {
namespace net {

#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
typedef SOCKET SocketFD;
#define NADJIEB_MJPEG_STREAMER_POLLFD WSAPOLLFD
#define NADJIEB_MJPEG_STREAMER_ERRNO WSAGetLastError()
#define NADJIEB_MJPEG_STREAMER_EWOULDBLOCK WSAEWOULDBLOCK
#define NADJIEB_MJPEG_STREAMER_SOCKET_ERROR SOCKET_ERROR
#define NADJIEB_MJPEG_STREAMER_INVALID_SOCKET INVALID_SOCKET

#elif defined NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX || defined NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
typedef int SocketFD;
#define NADJIEB_MJPEG_STREAMER_POLLFD pollfd
#define NADJIEB_MJPEG_STREAMER_ERRNO errno
#define NADJIEB_MJPEG_STREAMER_EWOULDBLOCK EAGAIN
#define NADJIEB_MJPEG_STREAMER_SOCKET_ERROR (-1)
#define NADJIEB_MJPEG_STREAMER_INVALID_SOCKET (-1)
#endif

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

static void panicIfUnexpected(
    bool condition,
    const std::string& message,
    const SocketFD& sockfd = NADJIEB_MJPEG_STREAMER_INVALID_SOCKET) {
    if (condition) {
        if (sockfd != NADJIEB_MJPEG_STREAMER_INVALID_SOCKET) {
            closeSocket(sockfd);
        }
        throw std::runtime_error(message + " - Error Code: " + std::to_string(NADJIEB_MJPEG_STREAMER_ERRNO));
    }
}

static void initSocket() {
#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
    WSAData wsaData;
    auto res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    panicIfUnexpected(res != 0, "initSocket() failed");
#elif defined NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX || defined NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
    auto res = signal(SIGPIPE, SIG_IGN);
    panicIfUnexpected(res == SIG_ERR, "initSocket() failed");
#endif
}

static SocketFD createSocket(int af, int type, int protocol) {
    SocketFD sockfd = ::socket(af, type, protocol);

    panicIfUnexpected(sockfd == NADJIEB_MJPEG_STREAMER_INVALID_SOCKET, "createSocket() failed", sockfd);

    return sockfd;
}

static void setSocketReuseAddress(SocketFD sockfd) {
    const int enable = 1;
    auto res = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));

    panicIfUnexpected(res == NADJIEB_MJPEG_STREAMER_SOCKET_ERROR, "setSocketReuseAddress() failed", sockfd);
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
    ip_addr.sin_port = htons((uint16_t)port);
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

static int readFromSocket(SocketFD socket, char* buffer, size_t length, int flags) {
#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
    return ::recv(socket, buffer, (int)length, flags);
#else
    return ::recv(socket, buffer, length, flags);
#endif
}

static int sendViaSocket(SocketFD socket, const char* buffer, size_t length, int flags) {
#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
    return ::send(socket, buffer, (int)length, flags);
#else
    return ::send(socket, buffer, length, flags);
#endif
}

static int pollSockets(NADJIEB_MJPEG_STREAMER_POLLFD* fds, size_t nfds, long timeout) {
#ifdef NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
    return WSAPoll(&fds[0], (ULONG)nfds, timeout);
#elif defined NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX || defined NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
    return poll(fds, nfds, timeout);
#endif
}
}  // namespace net
}  // namespace nadjieb

// #include <nadjieb/utils/non_copyable.hpp>


namespace nadjieb {
namespace utils {
class NonCopyable {
   public:
    NonCopyable(NonCopyable&&) = delete;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(NonCopyable&&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;

   protected:
    NonCopyable() = default;
    virtual ~NonCopyable() = default;
};
}  // namespace utils
}  // namespace nadjieb

// #include <nadjieb/utils/runnable.hpp>


namespace nadjieb {
namespace utils {
enum class State { UNSPECIFIED = 0, NEW, BOOTING, RUNNING, TERMINATING, TERMINATED };
class Runnable {
   public:
    State status() { return state_; }

    bool isRunning() { return (state_ == State::RUNNING); }

   protected:
    State state_ = State::NEW;
};
}  // namespace utils
}  // namespace nadjieb


#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace nadjieb {
namespace net {

struct OnMessageCallbackResponse {
    bool close_conn = false;
    bool end_listener = false;
};

using OnMessageCallback = std::function<OnMessageCallbackResponse(const SocketFD&, const std::string&)>;
using OnBeforeCloseCallback = std::function<void(const SocketFD&)>;

class Listener : public nadjieb::utils::NonCopyable, public nadjieb::utils::Runnable {
   public:
    virtual ~Listener() { stop(); }

    Listener& withOnMessageCallback(const OnMessageCallback& callback) {
        on_message_cb_ = callback;
        return *this;
    }

    Listener& withOnBeforeCloseCallback(const OnBeforeCloseCallback& callback) {
        on_before_close_cb_ = callback;
        return *this;
    }

    void stop() {
        end_listener_ = true;
        if (thread_listener_.joinable()) {
            thread_listener_.join();
        }
    }

    void runAsync(int port) { thread_listener_ = std::thread(&Listener::run, this, port); }

    void run(int port) {
        state_ = nadjieb::utils::State::BOOTING;
        panicIfUnexpected(on_message_cb_ == nullptr, "not setting on_message_cb");
        panicIfUnexpected(on_before_close_cb_ == nullptr, "not setting on_before_close_cb");

        end_listener_ = false;

        initSocket();
        listen_sd_ = createSocket(AF_INET, SOCK_STREAM, 0);
        setSocketReuseAddress(listen_sd_);
        setSocketNonblock(listen_sd_);
        bindSocket(listen_sd_, "0.0.0.0", port);
        listenOnSocket(listen_sd_, SOMAXCONN);

        fds_.emplace_back(NADJIEB_MJPEG_STREAMER_POLLFD{listen_sd_, POLLRDNORM, 0});

        std::string buff(4096, 0);

        state_ = nadjieb::utils::State::RUNNING;

        while (!end_listener_) {
            int socket_count = pollSockets(&fds_[0], fds_.size(), 100);

            panicIfUnexpected(socket_count == NADJIEB_MJPEG_STREAMER_SOCKET_ERROR, "pollSockets() failed");

            if (socket_count == 0) {
                continue;
            }

            size_t current_size = fds_.size();
            bool compress_array = false;
            for (size_t i = 0; i < current_size; ++i) {
                if (fds_[i].revents == 0) {
                    continue;
                }

                if (fds_[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    on_before_close_cb_(fds_[i].fd);
                    closeSocket(fds_[i].fd);
                    fds_[i].fd = NADJIEB_MJPEG_STREAMER_INVALID_SOCKET;
                    compress_array = true;
                    continue;
                }

                panicIfUnexpected(fds_[i].revents != POLLRDNORM, "revents != POLLRDNORM");

                if (fds_[i].fd == listen_sd_) {
                    do {
                        auto new_socket = acceptNewSocket(listen_sd_);
                        if (new_socket == NADJIEB_MJPEG_STREAMER_INVALID_SOCKET) {
                            panicIfUnexpected(
                                NADJIEB_MJPEG_STREAMER_ERRNO != NADJIEB_MJPEG_STREAMER_EWOULDBLOCK, "accept() failed");
                            break;
                        }

                        setSocketNonblock(new_socket);

                        fds_.emplace_back(NADJIEB_MJPEG_STREAMER_POLLFD{new_socket, POLLRDNORM, 0});
                    } while (true);
                } else {
                    std::string data;
                    bool close_conn = false;

                    do {
                        auto size = readFromSocket(fds_[i].fd, &buff[0], buff.size(), 0);
                        if (size == NADJIEB_MJPEG_STREAMER_SOCKET_ERROR) {
                            if (NADJIEB_MJPEG_STREAMER_ERRNO != NADJIEB_MJPEG_STREAMER_EWOULDBLOCK) {
                                std::cerr << "readFromSocket() failed" << std::endl;
                                close_conn = true;
                            }
                            break;
                        }

                        if (size == 0) {
                            close_conn = true;
                            break;
                        }

                        data += buff.substr(0, size);
                    } while (true);

                    if (!close_conn) {
                        auto resp = on_message_cb_(fds_[i].fd, data);
                        if (resp.close_conn) {
                            close_conn = resp.close_conn;
                        }

                        if (resp.end_listener) {
                            end_listener_ = resp.end_listener;
                        }
                    }

                    if (close_conn) {
                        on_before_close_cb_(fds_[i].fd);
                        closeSocket(fds_[i].fd);
                        fds_[i].fd = NADJIEB_MJPEG_STREAMER_INVALID_SOCKET;
                        compress_array = true;
                    }
                }
            }

            if (compress_array) {
                compress();
            }
        }

        closeAll();
    }

   private:
    SocketFD listen_sd_ = NADJIEB_MJPEG_STREAMER_INVALID_SOCKET;
    bool end_listener_ = true;
    std::vector<NADJIEB_MJPEG_STREAMER_POLLFD> fds_;
    OnMessageCallback on_message_cb_;
    OnBeforeCloseCallback on_before_close_cb_;
    std::thread thread_listener_;

    void compress() {
        for (auto it = fds_.begin(); it != fds_.end();) {
            if (it->fd == NADJIEB_MJPEG_STREAMER_INVALID_SOCKET) {
                it = fds_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void closeAll() {
        state_ = nadjieb::utils::State::TERMINATING;
        for (auto& pfd : fds_) {
            if (pfd.fd >= 0) {
                on_before_close_cb_(pfd.fd);
                closeSocket(pfd.fd);
            }
        }

        fds_.clear();
        destroySocket();
        state_ = nadjieb::utils::State::TERMINATED;
    }

    void panicIfUnexpected(bool condition, const std::string& message) {
        if (condition) {
            closeAll();
            throw std::runtime_error(message);
        }
    }
};
}  // namespace net
}  // namespace nadjieb

// #include <nadjieb/net/publisher.hpp>


// #include <nadjieb/net/socket.hpp>

// #include <nadjieb/utils/non_copyable.hpp>

// #include <nadjieb/utils/runnable.hpp>


#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nadjieb {
namespace net {
class Publisher : public nadjieb::utils::NonCopyable, public nadjieb::utils::Runnable {
   public:
    virtual ~Publisher() { stop(); }

    void start(int num_workers = 1) {
        state_ = nadjieb::utils::State::BOOTING;
        end_publisher_ = false;
        workers_.reserve(num_workers);
        for (auto i = 0; i < num_workers; ++i) {
            workers_.emplace_back(&Publisher::worker, this);
        }
        state_ = nadjieb::utils::State::RUNNING;
    }

    void stop() {
        state_ = nadjieb::utils::State::TERMINATING;
        end_publisher_ = true;
        condition_.notify_all();

        if (!workers_.empty()) {
            for (auto& w : workers_) {
                if (w.joinable()) {
                    w.join();
                }
            }
            workers_.clear();
        }

        path2clients_.clear();

        while (!payloads_.empty()) {
            payloads_.pop();
        }
        state_ = nadjieb::utils::State::TERMINATED;
    }

    void add(const std::string& path, const SocketFD& sockfd) {
        if (end_publisher_) {
            return;
        }

        const std::lock_guard<std::mutex> lock(p2c_mtx_);
        path2clients_[path].emplace_back(NADJIEB_MJPEG_STREAMER_POLLFD{sockfd, POLLWRNORM, 0});
    }

    void removeClient(const SocketFD& sockfd) {
        const std::lock_guard<std::mutex> lock(p2c_mtx_);
        for (auto it = path2clients_.begin(); it != path2clients_.end();) {
            it->second.erase(
                std::remove_if(
                    it->second.begin(), it->second.end(),
                    [&](const NADJIEB_MJPEG_STREAMER_POLLFD& pfd) { return pfd.fd == sockfd; }),
                it->second.end());

            if (it->second.empty()) {
                it = path2clients_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void enqueue(const std::string& path, const std::string& buffer) {
        if (end_publisher_) {
            return;
        }

        if (path2clients_.find(path) == path2clients_.end()) {
            return;
        }

        std::unique_lock<std::mutex> payloads_lock(payloads_mtx_);
        payloads_.emplace(path, buffer);
        payloads_lock.unlock();
        condition_.notify_one();
    }

    bool hasClient(const std::string& path) {
        const std::lock_guard<std::mutex> lock(p2c_mtx_);
        return path2clients_.find(path) != path2clients_.end() && !path2clients_[path].empty();
    }

   private:
    typedef std::pair<std::string, std::string> Payload;

    std::condition_variable condition_;
    std::vector<std::thread> workers_;
    std::queue<Payload> payloads_;
    std::unordered_map<std::string, std::vector<NADJIEB_MJPEG_STREAMER_POLLFD>> path2clients_;
    std::mutex cv_mtx_;
    std::mutex p2c_mtx_;
    std::mutex payloads_mtx_;
    bool end_publisher_ = true;

    void worker() {
        while (!end_publisher_) {
            std::unique_lock<std::mutex> cv_lock(cv_mtx_);

            condition_.wait(cv_lock, [&]() { return (end_publisher_ || !payloads_.empty()); });
            if (end_publisher_) {
                break;
            }

            std::unique_lock<std::mutex> payloads_lock(payloads_mtx_);

            Payload payload = std::move(payloads_.front());
            payloads_.pop();

            payloads_lock.unlock();
            cv_lock.unlock();

            std::string res_str
                = "--nadjiebmjpegstreamer\r\n"
                  "Content-Type: image/jpeg\r\n"
                  "Content-Length: "
                  + std::to_string(payload.second.size()) + "\r\n\r\n" + payload.second;

            const std::lock_guard<std::mutex> lock(p2c_mtx_);
            for (auto& client : path2clients_[payload.first]) {
                auto socket_count = pollSockets(&client, 1, 1);

                if (socket_count == NADJIEB_MJPEG_STREAMER_SOCKET_ERROR) {
                    throw std::runtime_error("pollSockets() failed\n");
                }

                if (socket_count == 0) {
                    continue;
                }

                if (client.revents != POLLWRNORM) {
                    throw std::runtime_error("revents != POLLWRNORM\n");
                }

                sendViaSocket(client.fd, res_str.c_str(), res_str.size(), 0);
            }
        }
    }
};
}  // namespace net
}  // namespace nadjieb

// #include <nadjieb/net/socket.hpp>

// #include <nadjieb/utils/non_copyable.hpp>


#include <string>

namespace nadjieb {
class MJPEGStreamer : public nadjieb::utils::NonCopyable {
   public:
    virtual ~MJPEGStreamer() { stop(); }

    void start(int port, int num_workers = 1) {
        publisher_.start(num_workers);
        listener_.withOnMessageCallback(on_message_cb_).withOnBeforeCloseCallback(on_before_close_cb_).runAsync(port);

        while (!isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void stop() {
        publisher_.stop();
        listener_.stop();
    }

    void publish(const std::string& path, const std::string& buffer) { publisher_.enqueue(path, buffer); }

    void setShutdownTarget(const std::string& target) { shutdown_target_ = target; }

    bool isRunning() { return (publisher_.isRunning() && listener_.isRunning()); }

    bool hasClient(const std::string& path) { return publisher_.hasClient(path); }

   private:
    nadjieb::net::Listener listener_;
    nadjieb::net::Publisher publisher_;
    std::string shutdown_target_ = "/shutdown";

    nadjieb::net::OnMessageCallback on_message_cb_ = [&](const nadjieb::net::SocketFD& sockfd,
                                                         const std::string& message) {
        nadjieb::net::HTTPRequest req(message);
        nadjieb::net::OnMessageCallbackResponse cb_res;

        if (req.getTarget() == shutdown_target_) {
            nadjieb::net::HTTPResponse shutdown_res;
            shutdown_res.setVersion(req.getVersion());
            shutdown_res.setStatusCode(200);
            shutdown_res.setStatusText("OK");
            auto shutdown_res_str = shutdown_res.serialize();

            nadjieb::net::sendViaSocket(sockfd, shutdown_res_str.c_str(), shutdown_res_str.size(), 0);

            publisher_.stop();

            cb_res.end_listener = true;
            return cb_res;
        }

        if (req.getMethod() != "GET") {
            nadjieb::net::HTTPResponse method_not_allowed_res;
            method_not_allowed_res.setVersion(req.getVersion());
            method_not_allowed_res.setStatusCode(405);
            method_not_allowed_res.setStatusText("Method Not Allowed");
            auto method_not_allowed_res_str = method_not_allowed_res.serialize();

            nadjieb::net::sendViaSocket(
                sockfd, method_not_allowed_res_str.c_str(), method_not_allowed_res_str.size(), 0);

            cb_res.close_conn = true;
            return cb_res;
        }

        nadjieb::net::HTTPResponse init_res;
        init_res.setVersion(req.getVersion());
        init_res.setStatusCode(200);
        init_res.setStatusText("OK");
        init_res.setValue("Connection", "close");
        init_res.setValue("Cache-Control", "no-cache, no-store, must-revalidate, pre-check=0, post-check=0, max-age=0");
        init_res.setValue("Pragma", "no-cache");
        init_res.setValue("Content-Type", "multipart/x-mixed-replace; boundary=nadjiebmjpegstreamer");
        auto init_res_str = init_res.serialize();

        nadjieb::net::sendViaSocket(sockfd, init_res_str.c_str(), init_res_str.size(), 0);

        publisher_.add(req.getTarget(), sockfd);

        return cb_res;
    };

    nadjieb::net::OnBeforeCloseCallback on_before_close_cb_
        = [&](const nadjieb::net::SocketFD& sockfd) { publisher_.removeClient(sockfd); };
};
}  // namespace nadjieb
