#include <doctest/doctest.h>

#include <nadjieb/net/listener.hpp>

TEST_SUITE("socket") {
    TEST_CASE("panicIfUnexpected") {
        nadjieb::net::Listener listener;
        CHECK_THROWS_WITH(listener.run(1234), "not setting on_message_cb");
    }
}
