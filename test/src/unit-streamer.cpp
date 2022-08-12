#include <doctest/doctest.h>
#include <yhirose/httplib.h>

#include <chrono>
#include <future>
#include <string>

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

                THEN("The streamer is alive but has no client for \"/foo\"") { CHECK(streamer.isRunning() == true); }
            }

            WHEN("The streamer stop") {
                streamer.stop();

                THEN("The streamer is not alive") { CHECK(streamer.isRunning() == false); }
            }
        }
    }

    TEST_CASE("publish image stream") {
        GIVEN("The streamer streams buffers") {
            const std::string buffer1 = "buffer1";
            const std::string buffer2 = "buffer2";
            std::string received_buffer1;
            std::string received_buffer2;
            bool ready = false;

            nadjieb::MJPEGStreamer streamer;
            streamer.start(1235);

            auto task = std::async(std::launch::async, [&]() {
                while (streamer.isRunning()) {
                    streamer.publish("/buffer1", buffer1);
                    streamer.publish("/buffer2", buffer2);
                    ready = true;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });

            WHEN("A client ready to receive image streams") {
                const std::string delimiter = "\r\n\r\n";
                httplib::Client cli("localhost", 1235);
                bool stop1 = false;
                bool stop2 = false;

                while (!ready) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                auto res1 = cli.Get("/buffer1", [&](const char* data, size_t data_length) {
                    received_buffer1.assign(data, data_length);
                    received_buffer1 = received_buffer1.substr(received_buffer1.find(delimiter) + delimiter.size());
                    stop1 = true;
                    return false;
                });

                auto res2 = cli.Get("/buffer2", [&](const char* data, size_t data_length) {
                    received_buffer2.assign(data, data_length);
                    received_buffer2 = received_buffer2.substr(received_buffer2.find(delimiter) + delimiter.size());
                    stop2 = true;
                    return false;
                });

                while (!stop1 || !stop2) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                streamer.stop();

                task.wait();

                THEN("The received buffers equal to the initial buffers") {
                    CHECK(received_buffer1 == buffer1);
                    CHECK(received_buffer2 == buffer2);
                }
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

                while (streamer.isRunning()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

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

    TEST_CASE("Not Found") {
        GIVEN("A streamer initialize") {
            nadjieb::MJPEGStreamer streamer;
            streamer.start(1240);

            CHECK(streamer.isRunning() == true);

            WHEN("Client request a non exist path") {
                httplib::Client cli("localhost", 1240);

                auto res = cli.Get("/foo");

                THEN("Connection closed") { CHECK(res->status == 404); }
            }
        }
    }

    TEST_CASE("Client disconnect when streamer publish buffer") {
        WHEN("A client request image stream and disconnect it") {
            nadjieb::MJPEGStreamer streamer;
            streamer.start(1238);

            bool ready = false;
            auto task = std::async(std::launch::async, [&]() {
                while (streamer.isRunning()) {
                    streamer.publish("/buffer", "buffer");
                    ready = true;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });

            while (!ready) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

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

    TEST_CASE("Streamer hasClient") {
        GIVEN("A streamer publishing message to a path") {
            nadjieb::MJPEGStreamer streamer;

            streamer.start(1239);

            auto publisher = std::async(std::launch::async, [&]() {
                while (streamer.isRunning()) {
                    streamer.publish("/buffer", "buffer");
                }
                streamer.publish("/buffer", "buffer");
            });

            WHEN("No client connected") {
                THEN("/buffer has no client") { CHECK(streamer.hasClient("/buffer") == false); }
            }

            httplib::Client cli("localhost", 1239);
            bool ready = false;

            auto client = std::async(std::launch::async, [&]() {
                auto res = cli.Get("/buffer", [&](const char*, size_t) {
                    ready = true;
                    return streamer.isRunning();
                });
            });

            WHEN("There is a client connected") {
                while (!ready) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                THEN("/buffer has client") { CHECK(streamer.hasClient("/buffer") == true); }
            }

            streamer.stop();
            client.wait();
            publisher.wait();
        }
    }
}
