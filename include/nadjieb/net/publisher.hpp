#pragma once

#include <nadjieb/net/socket.hpp>
#include <nadjieb/net/topic.hpp>
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

    void start(int num_workers = std::thread::hardware_concurrency()) {
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

        topics_.clear();
        path_by_client_.clear();

        while (!payloads_.empty()) {
            payloads_.pop();
        }
        state_ = nadjieb::utils::State::TERMINATED;
    }

    void add(const SocketFD& sockfd, const std::string& path) {
        if (end_publisher_) {
            return;
        }

        topics_[path].addClient(sockfd);

        std::unique_lock<std::mutex> lock(path_by_client_mtx_);
        path_by_client_[sockfd] = path;
    }

    bool pathExists(const std::string& path) { return (topics_.find(path) != topics_.end()); }

    void removeClient(const SocketFD& sockfd) {
        std::unique_lock<std::mutex> lock(path_by_client_mtx_);
        topics_[path_by_client_[sockfd]].removeClient(sockfd);

        path_by_client_.erase(sockfd);
    }

    void enqueue(const std::string& path, const std::string& buffer) {
        if (end_publisher_) {
            return;
        }

        topics_[path].setBuffer(buffer);

        for (const auto& client : topics_[path].getClients()) {
            if (topics_[path].getQueueSize(client.fd) > LIMIT_QUEUE_PER_CLIENT) {
                continue;
            }

            std::unique_lock<std::mutex> payloads_lock(payloads_mtx_);
            payloads_.emplace(path, client);
            topics_[path].increaseQueue(client.fd);
            payloads_lock.unlock();

            condition_.notify_one();
        }
    }

    bool hasClient(const std::string& path) { return topics_[path].hasClient(); }

   private:
    typedef std::pair<std::string, NADJIEB_MJPEG_STREAMER_POLLFD> Payload;

    std::condition_variable condition_;
    std::vector<std::thread> workers_;
    std::queue<Payload> payloads_;
    std::unordered_map<SocketFD, std::string> path_by_client_;
    std::unordered_map<std::string, Topic> topics_;
    std::mutex cv_mtx_;
    std::mutex path_by_client_mtx_;
    std::mutex payloads_mtx_;
    bool end_publisher_ = true;

    const static int LIMIT_QUEUE_PER_CLIENT = 5;

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
            topics_[payload.first].decreaseQueue(payload.second.fd);

            payloads_lock.unlock();
            cv_lock.unlock();

            auto buffer = topics_[payload.first].getBuffer();
            std::string res_str
                = "--nadjiebmjpegstreamer\r\n"
                  "Content-Type: image/jpeg\r\n"
                  "Content-Length: "
                  + std::to_string(buffer.size()) + "\r\n\r\n" + buffer;

            auto socket_count = pollSockets(&payload.second, 1, 1);

            if (socket_count == NADJIEB_MJPEG_STREAMER_SOCKET_ERROR) {
                throw std::runtime_error("pollSockets() failed\n");
            }

            if (socket_count == 0) {
                continue;
            }

            if (payload.second.revents != POLLWRNORM) {
                throw std::runtime_error("revents != POLLWRNORM\n");
            }

            sendViaSocket(payload.second.fd, res_str.c_str(), res_str.size(), 0);
        }
    }
};
}  // namespace net
}  // namespace nadjieb
