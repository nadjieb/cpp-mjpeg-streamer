/*
C++ MJPEG over HTTP Library
https://github.com/nadjieb/cpp-mjpeg-streamer

MIT License

Copyright (c) 2020 Muhammad Kamal Nadjieb

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

#include <csignal>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

/// The major version number
#define NADJIEB_MJPEG_STREAMER_VERSION_MAJOR 2

/// The minor version number
#define NADJIEB_MJPEG_STREAMER_VERSION_MINOR 0

/// The patch number
#define NADJIEB_MJPEG_STREAMER_VERSION_PATCH 0

/// The complete version number
#define NADJIEB_MJPEG_STREAMER_VERSION_CODE                                                                            \
    (NADJIEB_MJPEG_STREAMER_VERSION_MAJOR * 10000 + NADJIEB_MJPEG_STREAMER_VERSION_MINOR * 100 +                       \
     NADJIEB_MJPEG_STREAMER_VERSION_PATCH)

/// Version number as string
#define NADJIEB_MJPEG_STREAMER_VERSION_STRING "20000"

namespace nadjieb
{
constexpr int NUM_SEND_MUTICES = 100;
class MJPEGStreamer
{
  public:
    MJPEGStreamer() = default;
    virtual ~MJPEGStreamer()
    {
        stop();
    }

    MJPEGStreamer(MJPEGStreamer &&) = delete;
    MJPEGStreamer(const MJPEGStreamer &) = delete;
    MJPEGStreamer &operator=(MJPEGStreamer &&) = delete;
    MJPEGStreamer &operator=(const MJPEGStreamer &) = delete;

    void start(int port, int num_workers = 1)
    {
        ::signal(SIGPIPE, SIG_IGN);
        master_socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (master_socket_ < 0)
        {
            std::cerr << "ERROR: socket not created\n";
            exit(EXIT_FAILURE);
        }

        int yes = 1;
        if (::setsockopt(master_socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&yes), sizeof(yes)) < 0)
        {
            std::cerr << "ERROR: setsocketopt SO_REUSEADDR\n";
            exit(EXIT_FAILURE);
        }

        address_.sin_family = AF_INET;
        address_.sin_addr.s_addr = INADDR_ANY;
        address_.sin_port = htons(port);
        if (::bind(master_socket_, reinterpret_cast<struct sockaddr *>(&address_), sizeof(address_)) < 0)
        {
            std::cerr << "ERROR: bind\n";
            exit(EXIT_FAILURE);
        }

        if (::listen(master_socket_, 5) < 0)
        {
            std::cerr << "ERROR: listen\n";
            exit(EXIT_FAILURE);
        }

        for (auto i = 0; i < num_workers; ++i)
        {
            workers_.emplace_back(worker());
        }

        thread_listener_ = std::thread(listener());
    }

    void stop()
    {
        if (isAlive())
        {
            std::unique_lock<std::mutex> lock(payloads_mutex_);
            master_socket_ = -1;
            condition_.notify_all();
        }

        if (!workers_.empty())
        {
            for (auto &w : workers_)
            {
                if (w.joinable())
                {
                    w.join();
                }
            }
            workers_.clear();
        }

        if (!path2clients_.empty())
        {
            for (auto &p2c : path2clients_)
            {
                for (auto sd : p2c.second)
                {
                    ::close(sd);
                }
            }
            path2clients_.clear();
        }

        if (thread_listener_.joinable())
        {
            thread_listener_.join();
        }
    }

    void publish(const std::string &path, const std::string &buffer)
    {
        std::vector<int> clients;
        {
            std::unique_lock<std::mutex> lock(clients_mutex_);
            if ((path2clients_.find(path) == path2clients_.end()) || (path2clients_[path].empty()))
            {
                return;
            }
            clients = path2clients_[path];
        }

        for (auto i : clients)
        {
            std::unique_lock<std::mutex> lock(payloads_mutex_);
            payloads_.emplace(Payload({buffer, path, i}));
            condition_.notify_one();
        }
    }

    void setShutdownTarget(const std::string &target)
    {
        shutdown_target_ = target;
    }

    bool isAlive()
    {
        std::unique_lock<std::mutex> lock(payloads_mutex_);
        return master_socket_ > 0;
    }

  private:
    struct Payload
    {
        std::string buffer;
        std::string path;
        int sd;
    };

    int master_socket_ = -1;
    struct sockaddr_in address_;
    std::string shutdown_target_ = "/shutdown";

    std::thread thread_listener_;
    std::mutex clients_mutex_;
    std::mutex payloads_mutex_;
    std::array<std::mutex, NUM_SEND_MUTICES> send_mutices_;
    std::condition_variable condition_;

    std::vector<std::thread> workers_;
    std::queue<Payload> payloads_;
    std::unordered_map<std::string, std::vector<int>> path2clients_;

    std::function<void()> worker()
    {
        return [this]() {
            while (this->isAlive())
            {
                Payload payload;

                {
                    std::unique_lock<std::mutex> lock(this->payloads_mutex_);
                    this->condition_.wait(lock,
                                          [this]() { return this->master_socket_ < 0 || !this->payloads_.empty(); });
                    if ((this->master_socket_ < 0) && (this->payloads_.empty()))
                    {
                        return;
                    }
                    payload = std::move(this->payloads_.front());
                    this->payloads_.pop();
                }

                std::stringstream stream;
                stream << "--boundarydonotcross\r\nContent-Type: image/jpeg\r\nContent-Length: "
                       << payload.buffer.size() << "\r\n\r\n"
                       << payload.buffer;
                std::string msg = stream.str();

                int n;
                {
                    std::unique_lock<std::mutex> lock(this->send_mutices_[payload.sd % NUM_SEND_MUTICES]);
                    n = ::write(payload.sd, msg.c_str(), msg.size());
                }

                if (n < static_cast<int>(msg.size()))
                {
                    std::unique_lock<std::mutex> lock(this->clients_mutex_);
                    if (std::find(this->path2clients_[payload.path].begin(), this->path2clients_[payload.path].end(),
                                  payload.sd) != this->path2clients_[payload.path].end())
                    {
                        this->path2clients_[payload.path].erase(std::remove(this->path2clients_[payload.path].begin(),
                                                                            this->path2clients_[payload.path].end(),
                                                                            payload.sd),
                                                                this->path2clients_[payload.path].end());
                        ::close(payload.sd);
                    }
                }
            }
        };
    }

    std::function<void()> listener()
    {
        return [this]() {
            std::string header;
            header += "HTTP/1.1 200 OK\r\n";
            header += "Connection: close\r\n";
            header += "Cache-Control: no-cache, no-store, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n";
            header += "Pragma: no-cache\r\n";
            header += "Content-Type: multipart/x-mixed-replace; boundary=boundarydonotcross\r\n\r\n";

            int addrlen = sizeof(this->address_);

            fd_set fd;
            FD_ZERO(&fd);

            struct timeval to;
            to.tv_sec = 1;
            to.tv_usec = 0;

            auto master_socket = this->master_socket_;

            while (this->isAlive())
            {
                FD_SET(this->master_socket_, &fd);

                if (select(this->master_socket_ + 1, &fd, nullptr, nullptr, &to) > 0)
                {
                    auto new_socket =
                        ::accept(this->master_socket_, reinterpret_cast<struct sockaddr *>(&(this->address_)),
                                 reinterpret_cast<socklen_t *>(&addrlen));
                    if (new_socket < 0)
                    {
                        std::cerr << "ERROR: accept\n";
                        exit(EXIT_FAILURE);
                    }

                    std::string req(4096, 0);
                    ::read(new_socket, &req[0], req.size());

                    if (req.empty())
                    {
                        ::close(new_socket);
                        continue;
                    }

                    auto path = req.substr(req.find("GET") + 4, req.find("HTTP/") - req.find("GET") - 5);
                    if (path == this->shutdown_target_)
                    {
                        ::close(new_socket);
                        std::unique_lock<std::mutex> lock(this->payloads_mutex_);
                        this->master_socket_ = -1;
                        this->condition_.notify_all();
                        continue;
                    }

                    {
                        std::unique_lock<std::mutex> lock(this->send_mutices_[new_socket % NUM_SEND_MUTICES]);
                        ::write(new_socket, header.c_str(), header.size());
                    }

                    std::unique_lock<std::mutex> lock(this->clients_mutex_);
                    this->path2clients_[path].push_back(new_socket);
                }
            }

            ::shutdown(master_socket, 2);
        };
    }
};
} // namespace nadjieb
