#pragma once

#include <nadjieb/net/socket.hpp>
#include <nadjieb/utils/non_copyable.hpp>
#include <nadjieb/utils/runnable.hpp>

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
