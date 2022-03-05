#include <doctest/doctest.h>

#include <nadjieb/utils/version.hpp>

#include <string>

TEST_SUITE("Version") {
    TEST_CASE("Version") {
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_MAJOR == 3);
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_MINOR == 0);
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_PATCH == 0);
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_CODE == 30000);
        std::string expected_version = "3.0.0";
        CHECK(expected_version == NADJIEB_MJPEG_STREAMER_VERSION_STRING);
    }
}
