#include <doctest/doctest.h>

#define private public

#include <nadjieb/net/listener.hpp>

TEST_SUITE("socket") {
    TEST_CASE("panicIfUnexpected") {
        nadjieb::net::Listener listener;
        CHECK_THROWS_WITH(listener.panicIfUnexpected(true, "ERROR\n", true), "ERROR\n");
    }
}
