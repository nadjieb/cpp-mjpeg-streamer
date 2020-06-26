#include <opencv2/opencv.hpp>

#include <nadjieb/mjpeg_streamer.hpp>

// for convenience
using MJPEGStreamer = nadjieb::MJPEGStreamer;

int main()
{
    cv::VideoCapture cap(0);
    if (!cap.isOpened())
    {
        std::cerr << "VideoCapture not opened\n";
        exit(EXIT_FAILURE);
    }

    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 90};

    MJPEGStreamer streamer(8080);
    // By default 1 worker is used for streaming, if you want to use 4 workers
    // MJPEGStreamer streamer(8080, 4);

    streamer.start();
    while (true)
    {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty())
        {
            std::cerr << "frame not grabbed\n";
            exit(EXIT_FAILURE);
        }

        // http://localhost:8080/bgr
        std::vector<uchar> buff_bgr;
        cv::imencode(".jpg", frame, buff_bgr, params);
        streamer.publish("/bgr", std::string(buff_bgr.begin(), buff_bgr.end()));

        cv::Mat hsv;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        // http://localhost:8080/hsv
        std::vector<uchar> buff_hsv;
        cv::imencode(".jpg", hsv, buff_hsv, params);
        streamer.publish("/hsv", std::string(buff_hsv.begin(), buff_hsv.end()));
    
        // Visit /shutdown to break from the loop and graceful shutdown
        if (streamer.shutdownFromBrowser())
        {
            break;
        }
    }
    streamer.stop();
}
