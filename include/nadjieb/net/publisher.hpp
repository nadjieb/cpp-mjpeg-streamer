#pragma once

#include <nadjieb/net/socket.hpp>
#include <nadjieb/utils/non_copyable.hpp>
#include <nadjieb/utils/runnable.hpp>

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace nadjieb {
namespace net {
class Publisher : public nadjieb::utils::NonCopyable, public nadjieb::utils::Runnable {
   public:
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
        path2clients_[path].push_back(sockfd);
    }

    void removeClient(const SocketFD& sockfd) {
        const std::lock_guard<std::mutex> lock(p2c_mtx_);
        for (auto& p2c : path2clients_) {
            auto& clients = p2c.second;
            clients.erase(
                std::remove_if(
                    clients.begin(), clients.end(),
                    [&](const SocketFD& sfd) { return sfd == sockfd; }),
                clients.end());
        }
    }

    void enqueue(const std::string& path, const std::string& buffer) {
        if (end_publisher_) {
            return;
        }

        auto it = path2clients_.find(path);
        if (it == path2clients_.end()) {
            return;
        }

        for (auto& sockfd : it->second) {
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
