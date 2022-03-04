#include <doctest/doctest.h>

#include <nadjieb/utils/version.hpp>

#include <iostream>

TEST_SUITE("Version") {
    TEST_CASE("Version") {
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_MAJOR == 3);
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_MINOR == 0);
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_PATCH == 0);
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_CODE == 30000);
        std::cout << sizeof(NADJIEB_MJPEG_STREAMER_VERSION_STRING) << ' ' << sizeof("3.0.0") << std::endl;
        CHECK(NADJIEB_MJPEG_STREAMER_VERSION_STRING == "3.0.0");
    }
}
