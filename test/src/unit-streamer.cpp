#include <doctest/doctest.h>
#include <yhirose/httplib.h>

#include <nadjieb/mjpeg_streamer.hpp>

#include <chrono>
#include <future>

TEST_SUITE("streamer")
{
    TEST_CASE("stream state")
    {
        GIVEN("A streamer initialized")
        {
            nadjieb::MJPEGStreamer streamer;

            WHEN("The streamer initialized")
            {
                THEN("The streamer is not alive yet")
                {
                    CHECK(streamer.isAlive() == false);
                }
            }

            WHEN("The streamer start at port 1234 and publish a buffer")
            {
                streamer.start(1234);
                streamer.publish("/foo", "foo");

                THEN("The streamer is alive")
                {
                    CHECK(streamer.isAlive() == true);
                }
            }

            WHEN("The streamer stop")
            {
                streamer.stop();

                THEN("The streamer is not alive")
                {
                    CHECK(streamer.isAlive() == false);
                }
            }
        }
    }

    TEST_CASE("publish image stream")
    {
        GIVEN("A streamer publishing two different buffers")
        {
            const std::string buffer1 = "buffer1";
            const std::string buffer2 = "buffer2";

            auto task = std::async(std::launch::async, [&]() {
                nadjieb::MJPEGStreamer streamer;

                streamer.start(1235);

                while (streamer.isAlive())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    streamer.publish("/buffer1", buffer1);
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    streamer.publish("/buffer2", buffer2);
                    streamer.stop();
                }
            });

            WHEN("A client request image streams")
            {
                httplib::Client cli("localhost", 1235);

                std::string body1;
                cli.set_read_timeout(5, 0);
                auto res1 = cli.Get("/buffer1", [&](const char *data, size_t data_length) {
                    body1.assign(data, data_length);
                    return false;
                });

                std::string body2;
                cli.set_read_timeout(5, 0);
                auto res2 = cli.Get("/buffer2", [&](const char *data, size_t data_length) {
                    body2.assign(data, data_length);
                    return false;
                });

                task.wait();

                THEN("The received buffers equal to the initial buffers")
                {
                    std::string delimiter = "\r\n\r\n";
                    std::string received_buffer1 = body1.substr(body1.find(delimiter) + delimiter.size());
                    std::string received_buffer2 = body2.substr(body2.find(delimiter) + delimiter.size());

                    CHECK(received_buffer1 == buffer1);
                    CHECK(received_buffer2 == buffer2);
                }
            }
        }
    }

    TEST_CASE("Graceful Shutdown")
    {
        GIVEN("A streamer initialize with set shutdown target then start")
        {
            nadjieb::MJPEGStreamer streamer;
            streamer.setShutdownTarget("/stop");
            streamer.start(1236);

            CHECK(streamer.isAlive() == true);

            WHEN("Client request to graceful shutdown")
            {
                httplib::Client cli("localhost", 1236);

                auto res = cli.Get("/stop");

                THEN("The streamer is not alive")
                {
                    CHECK(res->status == 200);
                    CHECK(streamer.isAlive() == false);
                }
            }
        }
    }

    TEST_CASE("Method Not Allowed")
    {
        GIVEN("A streamer initialize")
        {
            nadjieb::MJPEGStreamer streamer;
            streamer.start(1237);

            CHECK(streamer.isAlive() == true);

            WHEN("Client request a POST")
            {
                httplib::Client cli("localhost", 1237);

                auto res = cli.Post("/foo");

                THEN("Connection closed")
                {
                    CHECK(res->status == 405);
                }
            }
        }
    }

    TEST_CASE("Client disconnect when streamer publish buffer")
    {
        GIVEN("A streamer initialized")
        {
            const std::string buffer = "buffer";
            nadjieb::MJPEGStreamer streamer;

            auto task = std::async(std::launch::async, [&]() {
                streamer.start(1238);

                while (streamer.isAlive())
                {
                    streamer.publish("/buffer", buffer);
                }
            });

            WHEN("A client request image stream and disconnect it")
            {
                httplib::Client cli("localhost", 1238);

                std::string body;
                auto res1 = cli.Get("/buffer", [&](const char *data, size_t data_length) {
                    body.assign(data, data_length);
                    return false;
                });

                THEN("Streamer is still alive")
                {
                    CHECK(body.empty() == false);
                    CHECK(streamer.isAlive());
                }
            }

            streamer.stop();
            task.wait();
        }
    }

    TEST_CASE("Panic If Unexpected")
    {
        GIVEN("A streamer initialized")
        {
            nadjieb::MJPEGStreamer streamer;

            WHEN("The streamer start")
            {
                THEN("It will throw exception")
                {
                    CHECK_THROWS_WITH(streamer.start(1238), "ERROR: bind\n");
                }
            }
        }
    }
}
