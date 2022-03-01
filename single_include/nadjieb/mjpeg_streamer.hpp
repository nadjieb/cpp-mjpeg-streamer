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

// #include <nadjieb/net/http_message.hpp>


#include <sstream>
#include <string>
#include <unordered_map>

namespace nadjieb {
namespace net {
struct HTTPMessage {
    HTTPMessage() = default;
    HTTPMessage(const std::string& message) { parse(message); }

    std::string start_line;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string serialize() const {
        const std::string delimiter = "\r\n";
        std::stringstream stream;

        stream << start_line << delimiter;

        for (const auto& header : headers) {
            stream << header.first << ": " << header.second << delimiter;
        }

        stream << delimiter << body;

        return stream.str();
    }

    void parse(const std::string& message) {
        const std::string delimiter = "\r\n";
        const std::string body_delimiter = "\r\n\r\n";

        start_line = message.substr(0, message.find(delimiter));

        auto raw_headers = message.substr(
            message.find(delimiter) + delimiter.size(),
            message.find(body_delimiter) - message.find(delimiter));

        while (raw_headers.find(delimiter) != std::string::npos) {
            auto header = raw_headers.substr(0, raw_headers.find(delimiter));
            auto key = header.substr(0, raw_headers.find(':'));
            auto value = header.substr(raw_headers.find(':') + 1, raw_headers.find(delimiter));
            while (value[0] == ' ') {
                value = value.substr(1);
            }
            headers[std::string(key)] = std::string(value);
            raw_headers = raw_headers.substr(raw_headers.find(delimiter) + delimiter.size());
        }

        body = message.substr(message.find(body_delimiter) + body_delimiter.size());
    }

    std::string target() const {
        std::string result(start_line.c_str() + start_line.find(' ') + 1);
        result = result.substr(0, result.find(' '));
        return std::string(result);
    }

    std::string method() const { return start_line.substr(0, start_line.find(' ')); }
};
}  // namespace net
}  // namespace nadjieb

// #include <nadjieb/net/listener.hpp>


// #include <nadjieb/net/socket.hpp>


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

// #include <nadjieb/utils/helper.hpp>


#include <iostream>

namespace nadjieb {
static void logError(const std::string& error_message) {
    std::cerr << error_message;
}
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


#include <errno.h>
#include <poll.h>

#include <functional>
#include <stdexcept>
#include <thread>
#include <vector>

namespace nadjieb {
namespace net {

struct OnMessageCallbackResponse {
    bool close_conn = false;
    bool end_listener = false;
};

using OnMessageCallback
    = std::function<OnMessageCallbackResponse(const SocketFD&, const std::string&)>;
using OnBeforeCloseCallback = std::function<void(const SocketFD&)>;

class Listener : public nadjieb::utils::NonCopyable {
   public:
    Listener& withOnMessageCallback(const OnMessageCallback& callback) {
        on_message_cb_ = callback;
        return *this;
    }

    Listener& withOnBeforeCloseCallback(const OnBeforeCloseCallback& callback) {
        on_before_close_cb_ = callback;
        return *this;
    }

    bool isAlive() { return !end_listener_; }

    void stop() {
        end_listener_ = true;
        if (thread_listener_.joinable()) {
            thread_listener_.join();
        }
    }

    void runAsync(int port) {
        end_listener_ = false;
        thread_listener_ = std::thread(&Listener::run, this, port);
    }

    void run(int port) {
        if (on_message_cb_ == nullptr) {
            throw std::runtime_error("not setting on_message_cb");
        }

        if (on_before_close_cb_ == nullptr) {
            throw std::runtime_error("not setting on_before_close_cb");
        }

        end_listener_ = false;

        initSocket();
        listen_sd_ = createSocket(AF_INET, SOCK_STREAM, 0);
        setSocketReuseAddress(listen_sd_);
        setSocketReusePort(listen_sd_);
        setSocketNonblock(listen_sd_);
        bindSocket(listen_sd_, "0.0.0.0", port);
        listenOnSocket(listen_sd_, SOMAXCONN);

        fds_.emplace_back(pollfd{listen_sd_, POLLIN, 0});

        std::string buff(4096, 0);

        while (!end_listener_) {
            int socket_count = ::poll(&fds_[0], fds_.size(), 100);

            if (socket_count == SOCKET_ERROR) {
                closeAll();
                throw std::runtime_error("poll() failed");
            }

            if (socket_count == 0) {
                continue;
            }

            auto current_size = fds_.size();
            bool compress_array = false;
            for (auto i = 0; i < current_size; ++i) {
                if (fds_[i].revents == 0) {
                    continue;
                }

                if (fds_[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    on_before_close_cb_(fds_[i].fd);
                    closeSocket(fds_[i].fd);
                    fds_[i].fd = -1;
                    compress_array = true;
                    continue;
                }

                if (fds_[i].revents != POLLIN) {
                    closeAll();
                    throw std::runtime_error("revents != POLLIN");
                }

                if (fds_[i].fd == listen_sd_) {
                    do {
                        auto new_socket = acceptNewSocket(listen_sd_);
                        if (new_socket < 0) {
                            if (errno != EWOULDBLOCK) {
                                closeAll();
                                throw std::runtime_error("accept() failed");
                            }
                            break;
                        }

                        fds_.emplace_back(pollfd{new_socket, POLLIN, 0});
                    } while (true);
                } else {
                    std::string data;
                    bool close_conn = false;

                    do {
                        auto size = readFromSocket(fds_[i].fd, &buff[0], buff.size(), 0);
                        if (size < 0) {
                            if (errno != EWOULDBLOCK) {
                                logError("recv() failed");
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
                        fds_[i].fd = -1;
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
    SocketFD listen_sd_ = SOCKET_INVALID;
    bool end_listener_ = true;
    std::vector<struct pollfd> fds_;
    OnMessageCallback on_message_cb_;
    OnBeforeCloseCallback on_before_close_cb_;
    std::thread thread_listener_;

    void compress() {
        for (auto it = fds_.begin(); it != fds_.end();) {
            if ((*it).fd == SOCKET_INVALID) {
                it = fds_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void closeAll() {
        for (auto& pfd : fds_) {
            if (pfd.fd >= 0) {
                on_before_close_cb_(pfd.fd);
                closeSocket(pfd.fd);
            }
        }

        fds_.clear();
    }
};
}  // namespace net
}  // namespace nadjieb

// #include <nadjieb/net/publisher.hpp>


// #include <nadjieb/net/socket.hpp>

// #include <nadjieb/utils/non_copyable.hpp>


#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace nadjieb {
namespace net {
class Publisher : public nadjieb::utils::NonCopyable {
   public:
    bool isAlive() { return !end_publisher_; }

    void start(int num_workers = 1) {
        end_publisher_ = false;
        workers_.reserve(num_workers);
        for (auto i = 0; i < num_workers; ++i) {
            workers_.emplace_back(&Publisher::worker, this);
        }
    }

    void stop() {
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
    }

    void add(const std::string& path, const SocketFD& sockfd) {
        if (end_publisher_) {
            return;
        }

        const std::lock_guard<std::mutex> lock(p2c_mtx_);
        path2clients_[path].push_back(sockfd);
    }

    void removeClient(const SocketFD& sockfd) {
        const std::lock_guard<std::mutex> lock(p2c_mtx_);
        for (auto& p2c : path2clients_) {
            auto& clients = p2c.second;
            if (std::find(clients.begin(), clients.end(), sockfd) != clients.end()) {
                clients.erase(std::remove(clients.begin(), clients.end(), sockfd), clients.end());
            }
        }
    }

    void enqueue(const std::string& path, const std::string& buffer) {
        if (end_publisher_) {
            return;
        }

        for (auto& sockfd : path2clients_[path]) {
            std::unique_lock<std::mutex> payloads_lock(payloads_mtx_);
            payloads_.emplace(buffer, path, sockfd);
            payloads_lock.unlock();
            condition_.notify_one();
        }
    }

    bool hasClient(const std::string& path) {
        const std::lock_guard<std::mutex> lock(p2c_mtx_);
        return path2clients_.find(path) != path2clients_.end() && !path2clients_[path].empty();
    }

   private:
    struct Payload {
        std::string buffer;
        std::string path;
        SocketFD sockfd;

        Payload(const std::string& b, const std::string& p, const SocketFD& s)
            : buffer(b), path(p), sockfd(s) {}
    };

    std::condition_variable condition_;
    std::vector<std::thread> workers_;
    std::queue<Payload> payloads_;
    std::unordered_map<std::string, std::vector<SocketFD>> path2clients_;
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

            HTTPMessage res;
            res.start_line = "--boundarydonotcross";
            res.headers["Content-Type"] = "image/jpeg";
            res.headers["Content-Length"] = std::to_string(payload.buffer.size());
            res.body = payload.buffer;

            auto res_str = res.serialize();

            struct pollfd psd;
            psd.fd = payload.sockfd;
            psd.events = POLLOUT;

            auto socket_count = ::poll(&psd, 1, 1);

            if (socket_count == SOCKET_ERROR) {
                throw std::runtime_error("poll() failed\n");
            }

            if (socket_count == 0) {
                continue;
            }

            if (psd.revents != POLLOUT) {
                throw std::runtime_error("revents != POLLOUT\n");
            }

            sendViaSocket(payload.sockfd, res_str.c_str(), res_str.size(), 0);
        }
    }
};
}  // namespace net
}  // namespace nadjieb

// #include <nadjieb/net/socket.hpp>

// #include <nadjieb/utils/non_copyable.hpp>


#include <string>

#include <chrono>
#include <thread>

namespace nadjieb {
class MJPEGStreamer : public nadjieb::utils::NonCopyable {
   public:
    void start(int port, int num_workers = 1) {
        publisher_.start(num_workers);
        listener_.withOnMessageCallback(on_message_cb_)
            .withOnBeforeCloseCallback(on_before_close_cb_)
            .runAsync(port);
    }

    void stop() {
        publisher_.stop();
        listener_.stop();
    }

    void publish(const std::string& path, const std::string& buffer) {
        publisher_.enqueue(path, buffer);
    }

    void setShutdownTarget(const std::string& target) { shutdown_target_ = target; }

    bool isAlive() { return (listener_.isAlive() && publisher_.isAlive()); }

    bool hasClient(const std::string& path) { return publisher_.hasClient(path); }

   private:
    nadjieb::net::Listener listener_;
    nadjieb::net::Publisher publisher_;
    std::string shutdown_target_ = "/shutdown";

    nadjieb::net::OnMessageCallback on_message_cb_ = [&](const nadjieb::net::SocketFD& sockfd,
                                                         const std::string& message) {
        nadjieb::net::HTTPMessage req(message);
        nadjieb::net::OnMessageCallbackResponse res;

        if (req.target() == "/shutdown") {
            nadjieb::net::HTTPMessage shutdown_res;
            shutdown_res.start_line = "HTTP/1.1 200 OK";
            auto shutdown_res_str = shutdown_res.serialize();

            nadjieb::net::sendViaSocket(
                sockfd, shutdown_res_str.c_str(), shutdown_res_str.size(), 0);

            publisher_.stop();

            res.end_listener = true;
            return res;
        }

        if (req.method() != "GET") {
            nadjieb::net::HTTPMessage method_not_allowed_res;
            method_not_allowed_res.start_line = "HTTP/1.1 405 Method Not Allowed";
            auto method_not_allowed_res_str = method_not_allowed_res.serialize();

            nadjieb::net::sendViaSocket(
                sockfd, method_not_allowed_res_str.c_str(), method_not_allowed_res_str.size(), 0);

            res.close_conn = true;
            return res;
        }

        nadjieb::net::HTTPMessage init_res;
        init_res.start_line = "HTTP/1.1 200 OK";
        init_res.headers["Connection"] = "close";
        init_res.headers["Cache-Control"]
            = "no-cache, no-store, must-revalidate, pre-check=0, post-check=0, max-age=0";
        init_res.headers["Pragma"] = "no-cache";
        init_res.headers["Content-Type"] = "multipart/x-mixed-replace; boundary=boundarydonotcross";
        auto init_res_str = init_res.serialize();

        nadjieb::net::sendViaSocket(sockfd, init_res_str.c_str(), init_res_str.size(), 0);

        publisher_.add(req.target(), sockfd);

        return res;
    };

    nadjieb::net::OnBeforeCloseCallback on_before_close_cb_
        = [&](const nadjieb::net::SocketFD& sockfd) { publisher_.removeClient(sockfd); };
};
}  // namespace nadjieb
