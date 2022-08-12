#pragma once

#include <nadjieb/net/socket.hpp>

#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace nadjieb {
namespace net {
class Topic {
   public:
    void setBuffer(const std::string& buffer) {
        std::unique_lock lock(buffer_mtx_);
        buffer_ = buffer;
    }

    std::string getBuffer() {
        std::shared_lock lock(buffer_mtx_);
        return buffer_;
    }

    void addClient(const SocketFD& sockfd) {
        std::unique_lock client_lock(client_by_sockfd_mtx_);
        client_by_sockfd_[sockfd] = NADJIEB_MJPEG_STREAMER_POLLFD{sockfd, POLLWRNORM, 0};

        std::unique_lock queue_size_lock(queue_size_by_sockfd__mtx_);
        queue_size_by_sockfd_[sockfd] = 0;
    }

    void removeClient(const SocketFD& sockfd) {
        std::unique_lock lock(client_by_sockfd_mtx_);
        client_by_sockfd_.erase(sockfd);

        std::unique_lock queue_size_lock(queue_size_by_sockfd__mtx_);
        queue_size_by_sockfd_.erase(sockfd);
    }

    bool hasClient() {
        std::shared_lock lock(client_by_sockfd_mtx_);
        return !client_by_sockfd_.empty();
    }

    std::vector<NADJIEB_MJPEG_STREAMER_POLLFD> getClients() {
        std::shared_lock lock(client_by_sockfd_mtx_);

        std::vector<NADJIEB_MJPEG_STREAMER_POLLFD> clients;
        for (const auto& client : client_by_sockfd_) {
            clients.push_back(client.second);
        }

        return clients;
    }

    int getQueueSize(const SocketFD& sockfd) {
        std::shared_lock queue_size_lock(queue_size_by_sockfd__mtx_);
        return queue_size_by_sockfd_[sockfd];
    }

    void increaseQueue(const SocketFD& sockfd) {
        std::unique_lock queue_size_lock(queue_size_by_sockfd__mtx_);
        ++queue_size_by_sockfd_[sockfd];
    }

    void decreaseQueue(const SocketFD& sockfd) {
        std::unique_lock queue_size_lock(queue_size_by_sockfd__mtx_);
        --queue_size_by_sockfd_[sockfd];
    }

   private:
    std::string buffer_;
    std::shared_mutex buffer_mtx_;

    std::unordered_map<SocketFD, NADJIEB_MJPEG_STREAMER_POLLFD> client_by_sockfd_;
    std::shared_mutex client_by_sockfd_mtx_;

    std::unordered_map<SocketFD, int> queue_size_by_sockfd_;
    std::shared_mutex queue_size_by_sockfd__mtx_;
};
}  // namespace net
}  // namespace nadjieb
