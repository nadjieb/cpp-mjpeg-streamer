#include <doctest/doctest.h>

#include <nadjieb/detail/http_message.hpp>

#include <string>

TEST_SUITE("HTTPMessage") {
    TEST_CASE("HTTP Response Message") {
        GIVEN("A raw response message") {
            const std::string raw
                = "HTTP/1.1 200 OK\r\n"
                  "Date: Mon, 27 Jul 2009 12:28:53 GMT\r\n"
                  "Server: Apache/2.2.14\r\n"
                  "Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT\r\n"
                  "Content-Length: 88\r\n"
                  "Content-Type: text/html\r\n"
                  "Connection: Closed\r\n"
                  "\r\n"
                  "<html>"
                  "<body>"
                  "<h1>Hello, World!</h1>"
                  "</body>"
                  "</html>";

            WHEN("Serialize a HTTPMessage") {
                nadjieb::HTTPMessage res;
                res.start_line = "HTTP/1.1 200 OK";
                res.headers["Date"] = "Mon, 27 Jul 2009 12:28:53 GMT";
                res.headers["Server"] = "Apache/2.2.14";
                res.headers["Last-Modified"] = "Wed, 22 Jul 2009 19:15:56 GMT";
                res.headers["Content-Length"] = "88";
                res.headers["Content-Type"] = "text/html";
                res.headers["Connection"] = "Closed";
                res.body
                    = "<html>"
                      "<body>"
                      "<h1>Hello, World!</h1>"
                      "</body>"
                      "</html>";

                auto serialized = res.serialize();

                THEN("The size is equal with raw response message") {
                    CHECK(serialized.size() == raw.size());
                }
            }
        }
    }

    TEST_CASE("HTTP Request Message") {
        GIVEN("A raw request message") {
            const std::string raw
                = "POST /cgi-bin/process.cgi HTTP/1.1\r\n"
                  "User-Agent: Mozilla/4.0\r\n"
                  "Server: Apache/2.2.14\r\n"
                  "Host: www.tutorialspoint.com\r\n"
                  "Content-Type: application/x-www-form-urlencoded\r\n"
                  "Content-Length: 50\r\n"
                  "Accept-Language: en-us\r\n"
                  "Accept-Encoding: gzip, deflate\r\n"
                  "Connection: Keep-Alive\r\n"
                  "\r\n"
                  "licenseID=string&content=string&/paramsXML=string";
            WHEN("HTTPMessage parse a raw request message") {
                nadjieb::HTTPMessage req(raw);

                THEN("The HTTPMessage equal to the raw message") {
                    CHECK(req.start_line == "POST /cgi-bin/process.cgi HTTP/1.1");
                    CHECK(req.method() == "POST");
                    CHECK(req.target() == "/cgi-bin/process.cgi");
                    CHECK(req.headers.size() == 8);
                    CHECK(req.headers["User-Agent"] == "Mozilla/4.0");
                    CHECK(req.headers["Server"] == "Apache/2.2.14");
                    CHECK(req.headers["Host"] == "www.tutorialspoint.com");
                    CHECK(req.headers["Content-Type"] == "application/x-www-form-urlencoded");
                    CHECK(req.headers["Content-Length"] == "50");
                    CHECK(req.headers["Accept-Language"] == "en-us");
                    CHECK(req.headers["Accept-Encoding"] == "gzip, deflate");
                    CHECK(req.headers["Connection"] == "Keep-Alive");
                    CHECK(req.body == "licenseID=string&content=string&/paramsXML=string");
                }
            }
        }
    }
}
