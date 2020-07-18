#include <chrono>
#include <future>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "curl/curl.h"
#include <nadjieb/mjpeg_streamer.hpp>
#include <opencv2/opencv.hpp>

// for convenience
using MJPEGStreamer = nadjieb::MJPEGStreamer;

// Uncomment to turn on the test's debug messages
// #define DEBUG

// Curl writefunction to be passed as a parameter
// can't ever expect to get the whole image in one piece,
// every router / hub is entitled to fragment it into parts
// (like 1-8k at a time), so insert the part at the end of our stream.
size_t write_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    std::vector<uchar> *stream = (std::vector<uchar> *)userdata;
    size_t count = size * nmemb;
    stream->insert(stream->end(), ptr, ptr + count);
    return count;
}

// function to retrieve an image from the web as cv::Mat data type
cv::Mat curlImg(const char *img_url, int timeout = 10)
{
    // Curl objects
    CURL *curl_handle;
    CURLcode res;
    cv::Mat image;
    std::vector<uchar> stream;

    // Start the curl session
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    // Set options
    curl_easy_setopt(curl_handle, CURLOPT_URL, img_url);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &stream);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
    res = curl_easy_perform(curl_handle);

    // Check for errors
    if (res != CURLE_OK)
    {
#ifdef DEBUG
        std::cout << "Image downloading failed: " << curl_easy_strerror(res) << std::endl;
#endif
    }

    // Free resources
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    // Clean up data and http headers
    stream.erase(stream.begin(), stream.begin() + 74);

#ifdef DEBUG
    std::cout << stream.data() << std::endl;
#endif

    // Return image 'keep-as-is'
    image = cv::imdecode(stream, -1);
    return image;
}

// Calculates if two images are the same or not
static bool ImagesAreEqual(const cv::Mat &a, const cv::Mat &b)
{
    if ((a.rows != b.rows) || (a.cols != b.cols))
    {
        return false;
    }

    cv::Scalar s = sum(a - b);
    return ((s[0] == 0) && (s[1] == 0) && (s[2] == 0));
}

// Threaded task for downloading images while non-blocking
int verifyImageTask(cv::Mat original, const char *img_url, int timeout = 10)
{
    cv::Mat downloaded = curlImg(img_url, timeout);

    // Check if downloaded images are the same as they were to start as
    if (ImagesAreEqual(original, downloaded) == false)
    {
        std::cout << "MJPEG streamer module unit tests failed" << std::endl;
        std::cout << img_url << " image was not the same after download" << std::endl;
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    std::cout << "Running unit tests for mjpeg streamer..." << std::endl;

    // Start the streamer
    MJPEGStreamer streamer;
    streamer.start(8080);

    // Read test images from file system
    cv::Mat testImageBGR, testImageHSV;
    testImageBGR = cv::imread("./lena.png", cv::IMREAD_COLOR);
    if (!testImageBGR.data)
    {
        std::cout << "Could not open or find the test image" << std::endl;
        std::cout << "MJPEG streamer module unit tests failed" << std::endl;
        return -1;
    }
    cv::cvtColor(testImageBGR, testImageHSV, cv::COLOR_BGR2HSV);

    // Download images from streamer non-blocking
    auto task1 = std::async(verifyImageTask, testImageBGR, "http://localhost:8080/bgr", 10);
    auto task2 = std::async(verifyImageTask, testImageHSV, "http://localhost:8080/hsv", 10);

    // Wait a little
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(2s);

    // http://localhost:8080/bgr
    std::vector<uchar> buff_bgr;
    cv::imencode(".png", testImageBGR, buff_bgr);
    streamer.publish("/bgr", std::string(buff_bgr.begin(), buff_bgr.end()));
#ifdef DEBUG
    std::cout << "BGR image live" << std::endl;
#endif

    // http://localhost:8080/hsv
    std::vector<uchar> buff_hsv;
    cv::imencode(".png", testImageHSV, buff_hsv);
    streamer.publish("/hsv", std::string(buff_hsv.begin(), buff_hsv.end()));
#ifdef DEBUG
    std::cout << "HSV image live" << std::endl;
#endif

    // Finish
    streamer.stop();
#ifdef DEBUG
    std::cout << "Streamer down" << std::endl;
#endif

    // Check for any errors
    if ((task1.get() < 0) || (task2.get() < 0))
    {
        return -2;
    }

    // Otherwise passed
    std::cout << "MJPEG streamer unit tests passed" << std::endl;
    return 0;
}
