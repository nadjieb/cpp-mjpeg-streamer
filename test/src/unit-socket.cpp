#include <doctest/doctest.h>

#include <nadjieb/net/socket.hpp>

TEST_SUITE("socket") {
    TEST_CASE("panicIfUnexpected") {
        nadjieb::net::initSocket();
        auto sockfd = nadjieb::net::createSocket(AF_INET, SOCK_STREAM, 0);
        CHECK_THROWS(nadjieb::net::panicIfUnexpected(true, "ERROR", sockfd));
    }
}
