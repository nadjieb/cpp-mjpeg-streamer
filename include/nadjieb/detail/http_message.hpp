#pragma once

#include <sstream>
#include <string>
#include <unordered_map>

namespace nadjieb {
struct HTTPMessage {
    HTTPMessage() = default;
    HTTPMessage(const std::string& message) { parse(message); }

    std::string start_line;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string serialize() const {
        const std::string delimiter = "\r\n";
        std::stringstream stream;

        stream << start_line << delimiter;

        for (const auto& header : headers) {
            stream << header.first << ": " << header.second << delimiter;
        }

        stream << delimiter << body;

        return stream.str();
    }

    void parse(const std::string& message) {
        const std::string delimiter = "\r\n";
        const std::string body_delimiter = "\r\n\r\n";

        start_line = message.substr(0, message.find(delimiter));

        auto raw_headers = message.substr(
            message.find(delimiter) + delimiter.size(),
            message.find(body_delimiter) - message.find(delimiter));

        while (raw_headers.find(delimiter) != std::string::npos) {
            auto header = raw_headers.substr(0, raw_headers.find(delimiter));
            auto key = header.substr(0, raw_headers.find(':'));
            auto value = header.substr(raw_headers.find(':') + 1, raw_headers.find(delimiter));
            while (value[0] == ' ') {
                value = value.substr(1);
            }
            headers[std::string(key)] = std::string(value);
            raw_headers = raw_headers.substr(raw_headers.find(delimiter) + delimiter.size());
        }

        body = message.substr(message.find(body_delimiter) + body_delimiter.size());
    }

    std::string target() const {
        std::string result(start_line.c_str() + start_line.find(' ') + 1);
        result = result.substr(0, result.find(' '));
        return std::string(result);
    }

    std::string method() const { return start_line.substr(0, start_line.find(' ')); }
};
}  // namespace nadjieb
