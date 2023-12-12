#pragma once

// Standard Includes
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// External Includes
#include <mjpeg_streamer.hpp>
#include <opencv2/opencv.hpp>

namespace MULTI_THREADING_JPG_STREAMER {
enum updateFrameEvents {
    FRAME_UPDATE_REFUSED_STREAMER_NOT_RUNNING = 1,
    FRAME_UPDATE_REFUSED_FRAME_IS_EMPTY = 2,
    FRAME_UPDATE_AND_DIRECT_PUBLISH_SUCCESSFUL = 3,
    FRAME_UPDATE_SUCCESSFUL_NO_DIRECT_PUBLISH = 4
};

class Streamer {
   private:
    nadjieb::MJPEGStreamer MJPEGStreamer;

    std::mutex frameUpdateMutex;
    std::vector<uchar> encodedFrame;

    int framerateThrottling;

    std::thread streamingThread;

    std::atomic<bool> keepStreamerRunning = true;

    std::vector<int> encodingOptions = {cv::IMWRITE_JPEG_QUALITY, 90};

    void publishFrame() {
        try {
            std::lock_guard<std::mutex> lock(frameUpdateMutex);
            MJPEGStreamer.publish("/", std::string(encodedFrame.begin(), encodedFrame.end()));
        } catch (const std::exception&) {
            stopStreaming();
        }
    }

    bool threadIsRunning() { return streamingThread.joinable(); }

    bool streamerIsRunning() { return MJPEGStreamer.isRunning(); }

    void runStreaming() {
        while (keepStreamerRunning) {
            // Exit execution if MJPEGStreamer is no longer running
            if (streamerIsRunning() == false) {
                break;
            }

            // Publish the frame if throttling is desired
            if (framerateThrottling > 0) {
                publishFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / framerateThrottling));
            }
        }

        // Stop the streamer
        if (MJPEGStreamer.isRunning()) {
            MJPEGStreamer.stop();
        }
    }

   public:
    Streamer() {}

    ~Streamer() { stopStreaming(); }

    /**
     * Checks if the streaming thread is active.
     *
     * @return true if the streaming thread is active, otherwise false.
     */
    bool streamingIsRunning() { return threadIsRunning(); }

    /**
     * Updates the current frame used for streaming.
     *
     * @param frame OpenCV Mat representing a new frame.
     *
     * @throws std::runtime_error if the streaming thread is not running or the streamer is stopped.
     */
    int updateFrame(const cv::Mat& frame) {
        if (threadIsRunning() == true) {
            std::lock_guard<std::mutex> lock(frameUpdateMutex);

            if (frame.empty() == true) {
                return FRAME_UPDATE_REFUSED_FRAME_IS_EMPTY;
            } else {
                cv::imencode(".jpg", frame, encodedFrame, encodingOptions);
            }

            if (framerateThrottling == 0) {
                publishFrame();
                return FRAME_UPDATE_AND_DIRECT_PUBLISH_SUCCESSFUL;
            } else {
                return FRAME_UPDATE_SUCCESSFUL_NO_DIRECT_PUBLISH;
            }
        } else {
            return FRAME_UPDATE_REFUSED_STREAMER_NOT_RUNNING;
        }
    }

    /**
     * Starts a separate thread in which the streamer is executed.
     */
    void startStreaming(unsigned int port, unsigned int framerateThrottling = 0, unsigned int jpgQuality = 90) {
        // Check if the streamer is already running and if the parameters have changed
        if (threadIsRunning()) {
            const bool s1 = this->framerateThrottling != framerateThrottling;
            const bool s2 = this->encodingOptions[1] != jpgQuality;

            const bool parameterChanged = s1 || s2;

            if (parameterChanged) {
                stopStreaming();
            } else {
                return;
            }
        }

        // Set new parameters and start the streamer
        this->framerateThrottling = framerateThrottling;
        this->encodingOptions[1] = jpgQuality;

        MJPEGStreamer.start(port);

        streamingThread = std::thread(&Streamer::runStreaming, this);
    }

    /**
     * Interrupts the execution loop in the streamer thread and performs a join on the thread.
     */
    void stopStreaming() {
        keepStreamerRunning = false;

        if (streamingThread.joinable()) {
            streamingThread.join();
        }
    }
};

}  // namespace MULTI_THREADING_JPG_STREAMER
