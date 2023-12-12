#include <thread>
#include <vector>
#include "multiThreadingJpgStreamer.h"

class CameraSimulator {
   public:
    CameraSimulator(MULTI_THREADING_JPG_STREAMER::Streamer& streamer) : streamer_(streamer) {}

    void run() {
        while (true) {
            // Simulate capturing a frame from a mock-up camera
            cv::Mat mockupFrame(480, 640, CV_8UC3);
            mockupFrame.setTo(cv::Scalar(0, 255, 0));  // Green frame as a mock-up

            // Update the stream with the mock-up frame
            int updateResult = streamer_.updateFrame(mockupFrame);

            // Sleep for one second (simulating one frame per second)
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

   private:
    MULTI_THREADING_JPG_STREAMER::Streamer& streamer_;
};

int main() {
    // Create an instance of your Streamer class (global)
    MULTI_THREADING_JPG_STREAMER::Streamer streamer;

    // Start the streaming on a specified port (e.g., port 8080)
    unsigned int port = 8080;
    unsigned int framerateThrottling = 30;  // Set the desired frame rate
    unsigned int jpgQuality = 80;           // Set the JPEG quality

    streamer.startStreaming(port, framerateThrottling, jpgQuality);

    // Create a CameraSimulator instance that uses the global streamer
    CameraSimulator cameraSimulator(streamer);

    // Create a thread for the CameraSimulator
    std::thread cameraThread(&CameraSimulator::run, &cameraSimulator);

    // Wait for the camera thread to finish
    cameraThread.join();

    // Stop the streaming when done
    streamer.stopStreaming();

    return 0;
}
