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
