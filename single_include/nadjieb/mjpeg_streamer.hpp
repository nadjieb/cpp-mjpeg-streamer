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
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

// #include <nadjieb/detail/version.hpp>


/// The major version number
#define NADJIEB_MJPEG_STREAMER_VERSION_MAJOR 2

/// The minor version number
#define NADJIEB_MJPEG_STREAMER_VERSION_MINOR 0

/// The patch number
#define NADJIEB_MJPEG_STREAMER_VERSION_PATCH 0

/// The complete version number
#define NADJIEB_MJPEG_STREAMER_VERSION_CODE (NADJIEB_MJPEG_STREAMER_VERSION_MAJOR * 10000 + NADJIEB_MJPEG_STREAMER_VERSION_MINOR * 100 + NADJIEB_MJPEG_STREAMER_VERSION_PATCH)

/// Version number as string
#define NADJIEB_MJPEG_STREAMER_VERSION_STRING "2.0.0"


// #include <nadjieb/detail/http_message.hpp>


#include <sstream>
#include <string>
#include <unordered_map>

namespace nadjieb
{
struct HTTPMessage
{
    HTTPMessage() = default;
    HTTPMessage(const std::string &message)
    {
        parse(message);
    }

    std::string start_line;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string serialize() const
    {
        const std::string delimiter = "\r\n";
        std::stringstream stream;

        stream << start_line << delimiter;

        for (const auto &header : headers)
        {
            stream << header.first << ": " << header.second << delimiter;
        }

        stream << delimiter << body;

        return stream.str();
    }

    void parse(const std::string &message)
    {
        const std::string delimiter = "\r\n";
        const std::string body_delimiter = "\r\n\r\n";

        start_line = message.substr(0, message.find(delimiter));

        auto raw_headers = message.substr(message.find(delimiter) + delimiter.size(),
                                          message.find(body_delimiter) - message.find(delimiter));

        while (raw_headers.find(delimiter) != std::string::npos)
        {
            auto header = raw_headers.substr(0, raw_headers.find(delimiter));
            auto key = header.substr(0, raw_headers.find(':'));
            auto value = header.substr(raw_headers.find(':') + 1, raw_headers.find(delimiter));
            while (value[0] == ' ')
            {
                value = value.substr(1);
            }
            headers[std::string(key)] = std::string(value);
            raw_headers = raw_headers.substr(raw_headers.find(delimiter) + delimiter.size());
        }

        body = message.substr(message.find(body_delimiter) + body_delimiter.size());
    }

    std::string target() const
    {
        std::string result(start_line.c_str() + start_line.find(' ') + 1);
        result = result.substr(0, result.find(' '));
        return std::string(result);
    }

    std::string method() const
    {
        return start_line.substr(0, start_line.find(' '));
    }
};
} // namespace nadjieb


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
        panicIfUnexpected(master_socket_ < 0, "ERROR: socket not created\n");

        int yes = 1;
        auto res = ::setsockopt(master_socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&yes), sizeof(yes));
        panicIfUnexpected(res < 0, "ERROR: setsocketopt SO_REUSEADDR\n");

        address_.sin_family = AF_INET;
        address_.sin_addr.s_addr = INADDR_ANY;
        address_.sin_port = htons(port);
        res = ::bind(master_socket_, reinterpret_cast<struct sockaddr *>(&address_), sizeof(address_));
        panicIfUnexpected(res < 0, "ERROR: bind\n");

        res = ::listen(master_socket_, 5);
        panicIfUnexpected(res < 0, "ERROR: listen\n");

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
            payloads_.emplace(Payload{buffer, path, i});
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

                HTTPMessage res;
                res.start_line = "--boundarydonotcross";
                res.headers["Content-Type"] = "image/jpeg";
                res.headers["Content-Length"] = std::to_string(payload.buffer.size());
                res.body = payload.buffer;

                auto res_str = res.serialize();

                int n;
                {
                    std::unique_lock<std::mutex> lock(this->send_mutices_.at(payload.sd % NUM_SEND_MUTICES));
                    n = ::write(payload.sd, res_str.c_str(), res_str.size());
                }

                if (n < static_cast<int>(res_str.size()))
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
            HTTPMessage bad_request_res;
            bad_request_res.start_line = "HTTP/1.1 400 Bad Request";

            HTTPMessage shutdown_res;
            shutdown_res.start_line = "HTTP/1.1 200 OK";

            HTTPMessage method_not_allowed_res;
            method_not_allowed_res.start_line = "HTTP/1.1 405 Method Not Allowed";

            HTTPMessage init_res;
            init_res.start_line = "HTTP/1.1 200 OK";
            init_res.headers["Connection"] = "close";
            init_res.headers["Cache-Control"] =
                "no-cache, no-store, must-revalidate, pre-check=0, post-check=0, max-age=0";
            init_res.headers["Pragma"] = "no-cache";
            init_res.headers["Content-Type"] = "multipart/x-mixed-replace; boundary=boundarydonotcross";

            auto bad_request_res_str = bad_request_res.serialize();
            auto shutdown_res_str = shutdown_res.serialize();
            auto method_not_allowed_res_str = method_not_allowed_res.serialize();
            auto init_res_str = init_res.serialize();

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
                    this->panicIfUnexpected(new_socket < 0, "ERROR: accept\n");

                    std::string buff(4096, 0);
                    ::read(new_socket, &buff[0], buff.size());

                    HTTPMessage req(buff);

                    if (req.target() == this->shutdown_target_)
                    {
                        ::write(new_socket, shutdown_res_str.c_str(), shutdown_res_str.size());
                        ::close(new_socket);

                        std::unique_lock<std::mutex> lock(this->payloads_mutex_);
                        this->master_socket_ = -1;
                        this->condition_.notify_all();

                        continue;
                    }

                    if (req.method() != "GET")
                    {
                        ::write(new_socket, method_not_allowed_res_str.c_str(), method_not_allowed_res_str.size());
                        ::close(new_socket);
                        continue;
                    }

                    ::write(new_socket, init_res_str.c_str(), init_res_str.size());

                    std::unique_lock<std::mutex> lock(this->clients_mutex_);
                    this->path2clients_[req.target()].push_back(new_socket);
                }
            }

            ::shutdown(master_socket, 2);
        };
    }

    static void panicIfUnexpected(bool condition, const std::string &message)
    {
        if (condition)
        {
            throw std::runtime_error(message);
        }
    }
};
} // namespace nadjieb
