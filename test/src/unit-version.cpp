#include <doctest/doctest.h>

#include <nadjieb/detail/version.hpp>

#include <string>

TEST_SUITE("Version") {
    TEST_CASE("Version") {
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_MAJOR == 2);
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_MINOR == 0);
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_PATCH == 0);
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_CODE == 20000);
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_STRING == "2.0.0");
    }
}
