cmake_minimum_required(VERSION 3.1)
project(nadjieb_mjpeg_streamer_tests LANGUAGES CXX)

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_STANDARD 17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(OpenCV REQUIRED)
include_directories(../include/)

add_executable(tests ./tests.cpp)
target_compile_features(tests PRIVATE cxx_std_11)
target_link_libraries(tests
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
        curl
)
