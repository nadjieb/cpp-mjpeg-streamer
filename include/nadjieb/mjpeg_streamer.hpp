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

#include <nadjieb/utils/version.hpp>

#include <nadjieb/net/http_request.hpp>
#include <nadjieb/net/http_response.hpp>
#include <nadjieb/net/listener.hpp>
#include <nadjieb/net/publisher.hpp>
#include <nadjieb/net/socket.hpp>
#include <nadjieb/utils/non_copyable.hpp>

#include <string>

namespace nadjieb {
class MJPEGStreamer : public nadjieb::utils::NonCopyable {
   public:
    virtual ~MJPEGStreamer() { stop(); }

    void start(int port, int num_workers = std::thread::hardware_concurrency()) {
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

        if (!publisher_.pathExists(req.getTarget())) {
            nadjieb::net::HTTPResponse not_found_res;
            not_found_res.setVersion(req.getVersion());
            not_found_res.setStatusCode(404);
            not_found_res.setStatusText("Not Found");
            auto not_found_res_str = not_found_res.serialize();

            nadjieb::net::sendViaSocket(sockfd, not_found_res_str.c_str(), not_found_res_str.size(), 0);

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

        publisher_.add(sockfd, req.getTarget());

        return cb_res;
    };

    nadjieb::net::OnBeforeCloseCallback on_before_close_cb_
        = [&](const nadjieb::net::SocketFD& sockfd) { publisher_.removeClient(sockfd); };
};
}  // namespace nadjieb
