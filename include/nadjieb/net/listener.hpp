#pragma once

#include <nadjieb/net/socket.hpp>
#include <nadjieb/utils/helper.hpp>
#include <nadjieb/utils/non_copyable.hpp>

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
