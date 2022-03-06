#include <doctest/doctest.h>

#include <nadjieb/net/http_response.hpp>

#include <string>

TEST_SUITE("HTTPResponse") {
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

            WHEN("Serialize a HTTPResponse") {
                nadjieb::net::HTTPResponse res;
                res.setVersion("HTTP/1.1");
                res.setStatusCode(200);
                res.setStatusText("OK");
                res.setValue("Date", "Mon, 27 Jul 2009 12:28:53 GMT");
                res.setValue("Server", "Apache/2.2.14");
                res.setValue("Last-Modified", "Wed, 22 Jul 2009 19:15:56 GMT");
                res.setValue("Content-Length", "88");
                res.setValue("Content-Type", "text/html");
                res.setValue("Connection", "Closed");
                res.setBody("<html><body><h1>Hello, World!</h1></body></html>");

                auto serialized = res.serialize();

                THEN("The size is equal with raw response message") { CHECK(serialized.size() == raw.size()); }
            }
        }
    }
}
