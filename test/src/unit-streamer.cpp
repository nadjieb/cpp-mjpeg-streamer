#include <doctest/doctest.h>
#include <yhirose/httplib.h>

#include <chrono>
#include <future>
#include <string>

#define private public

#include <nadjieb/mjpeg_streamer.hpp>

TEST_SUITE("streamer") {
    TEST_CASE("stream state") {
        GIVEN("A streamer initialized") {
            nadjieb::MJPEGStreamer streamer;

            WHEN("The streamer initialized") {
                THEN("The streamer is not alive yet") { CHECK(streamer.isRunning() == false); }
            }

            WHEN("The streamer start at port 1234 and publish a buffer") {
                streamer.start(1234);
                streamer.publish("/foo", "foo");

                THEN("The streamer is alive but has no client for \"/foo\"") {
                    CHECK(streamer.isRunning() == true);
                }
            }

            WHEN("The streamer stop") {
                streamer.stop();

                THEN("The streamer is not alive") { CHECK(streamer.isRunning() == false); }
            }
        }
    }

    TEST_CASE("publish image stream") {
        GIVEN("streamer and clients") {
            nadjieb::MJPEGStreamer streamer;
            const std::string buffer1 = "buffer1";
            const std::string buffer2 = "buffer2";

            const std::string delimiter = "\r\n\r\n";
            std::string received_buffer1;
            std::string received_buffer2;

            WHEN("Clients request for image stream") {
                streamer.start(1235);

                auto client1 = std::async(std::launch::async, [&]() {
                    httplib::Client cli("localhost", 1235);
                    cli.Get("/buffer1", [&](const char* data, size_t data_length) {
                        std::string buff;
                        buff.assign(data, data_length);
                        received_buffer1 = buff.substr(buff.find(delimiter) + delimiter.size());
                        return false;
                    });
                });

                auto client2 = std::async(std::launch::async, [&]() {
                    httplib::Client cli("localhost", 1235);
                    cli.Get("/buffer2", [&](const char* data, size_t data_length) {
                        std::string buff;
                        buff.assign(data, data_length);
                        received_buffer2 = buff.substr(buff.find(delimiter) + delimiter.size());
                        return false;
                    });
                });

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                streamer.publish("/buffer1", buffer1);
                streamer.publish("/buffer2", buffer2);

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                client1.wait();
                client2.wait();

                THEN("The received buffers equal to the initial buffers") {
                    CHECK(received_buffer1 == buffer1);
                    CHECK(received_buffer2 == buffer2);
                }

                streamer.stop();
            }
        }
    }

    TEST_CASE("Graceful Shutdown") {
        GIVEN("A streamer initialize with set shutdown target then start") {
            nadjieb::MJPEGStreamer streamer;
            streamer.setShutdownTarget("/stop");
            streamer.start(1236);

            CHECK(streamer.isRunning() == true);

            WHEN("Client request to graceful shutdown") {
                httplib::Client cli("localhost", 1236);

                auto res = cli.Get("/stop");

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                THEN("The streamer is not alive") {
                    CHECK(res->status == 200);
                    CHECK(streamer.isRunning() == false);
                }
            }
        }
    }

    TEST_CASE("Method Not Allowed") {
        GIVEN("A streamer initialize") {
            nadjieb::MJPEGStreamer streamer;
            streamer.start(1237);

            CHECK(streamer.isRunning() == true);

            WHEN("Client request a POST") {
                httplib::Client cli("localhost", 1237);

                auto res = cli.Post("/foo");

                THEN("Connection closed") { CHECK(res->status == 405); }
            }
        }
    }

    TEST_CASE("Client disconnect when streamer publish buffer") {
        WHEN("A client request image stream and disconnect it") {
            nadjieb::MJPEGStreamer streamer;
            auto task = std::async(std::launch::async, [&]() {
                streamer.start(1238);
                while (streamer.isRunning()) {
                    streamer.publish("/buffer", "buffer");
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });

            std::this_thread::sleep_for(std::chrono::seconds(2));

            httplib::Client cli("localhost", 1238);

            std::string body;
            auto res = cli.Get("/buffer", [&](const char* data, size_t data_length) {
                body.assign(data, data_length);
                return false;
            });

            THEN("The streamer is still alive") {
                CHECK(body.empty() == false);
                CHECK(streamer.isRunning() == true);
            }

            streamer.stop();
            task.wait();
        }
    }
}
