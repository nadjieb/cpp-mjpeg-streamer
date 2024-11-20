cmake_minimum_required(VERSION 3.8)
project(nadjieb_mjpeg_streamer_examples LANGUAGES CXX)

find_package(nadjieb_mjpeg_streamer REQUIRED)
find_package(OpenCV 4.2 REQUIRED)

add_executable(example ./example.cpp)
target_compile_features(example PRIVATE cxx_std_17)
target_link_libraries(example
    PRIVATE
        nadjieb_mjpeg_streamer::nadjieb_mjpeg_streamer
        ${OpenCV_LIBS}
)
