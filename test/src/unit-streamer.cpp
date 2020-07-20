#include <doctest/doctest.h>

#include <nadjieb/mjpeg_streamer.hpp>

TEST_CASE("streamer")
{
    nadjieb::MJPEGStreamer streamer;
    CHECK(streamer.isAlive() == false);
}
