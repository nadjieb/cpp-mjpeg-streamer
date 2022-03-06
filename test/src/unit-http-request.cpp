#include <doctest/doctest.h>

#include <nadjieb/net/http_request.hpp>

TEST_SUITE("HTTPRequest") {
    TEST_CASE("HTTP Request Message") {
        GIVEN("A raw request message") {
            const char* raw
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
                  "licenseID=string&content=string&/paramsXML=string\n\n\n";
            WHEN("HTTPRequest parse a raw request message") {
                nadjieb::net::HTTPRequest req(raw);

                THEN("The HTTPRequest equal to the raw message") {
                    CHECK(req.getMethod() == "POST");
                    CHECK(req.getTarget() == "/cgi-bin/process.cgi");
                    CHECK(req.getVersion() == "HTTP/1.1");
                    CHECK(req.getValue("User-Agent") == "Mozilla/4.0");
                    CHECK(req.getValue("Server") == "Apache/2.2.14");
                    CHECK(req.getValue("Host") == "www.tutorialspoint.com");
                    CHECK(req.getValue("Content-Type") == "application/x-www-form-urlencoded");
                    CHECK(req.getValue("Content-Length") == "50");
                    CHECK(req.getValue("Accept-Language") == "en-us");
                    CHECK(req.getValue("Accept-Encoding") == "gzip, deflate");
                    CHECK(req.getValue("Connection") == "Keep-Alive");
                    CHECK(req.getValue("abc") == "");
                    CHECK(req.getBody() == "licenseID=string&content=string&/paramsXML=string\n\n\n");
                }
            }
        }
    }
}
