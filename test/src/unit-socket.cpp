#include <doctest/doctest.h>

#include <nadjieb/net/socket.hpp>

TEST_SUITE("socket") {
    TEST_CASE("panicIfUnexpected") {
        auto sockfd = nadjieb::net::createSocket(AF_INET, SOCK_STREAM, 0);
        CHECK_THROWS_WITH(nadjieb::net::panicIfUnexpected(true, "ERROR\n", sockfd), "ERROR\n");
    }
}
