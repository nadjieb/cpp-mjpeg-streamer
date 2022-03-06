#pragma once

#include <sstream>
#include <string>
#include <unordered_map>

// Reference https://developer.mozilla.org/en-US/docs/Web/HTTP/Messages#http_responses

namespace nadjieb {
namespace net {
class HTTPResponse {
   public:
    std::string serialize() {
        const std::string delimiter = "\r\n";
        std::stringstream stream;

        stream << version_ << ' ' << status_code_ << ' ' << status_text_ << delimiter;

        for (const auto& header : headers_) {
            stream << header.first << ": " << header.second << delimiter;
        }

        stream << delimiter << body_;

        return stream.str();
    }

    void setVersion(const std::string& version) { version_ = version; }
    void setStatusCode(const int& status_code) { status_code_ = status_code; }
    void setStatusText(const std::string& status_text) { status_text_ = status_text; }
    void setValue(const std::string& key, const std::string& value) { headers_[key] = value; }
    void setBody(const std::string& body) { body_ = body; }

   private:
    std::string version_;
    int status_code_;
    std::string status_text_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};
}  // namespace net
}  // namespace nadjieb
