cmake_minimum_required(VERSION 3.8)

project(DummyImportMinVer CXX)

find_package(nadjieb_mjpeg_streamer 3.0.0 REQUIRED)

add_executable(with_namespace_target main.cpp)
target_link_libraries(with_namespace_target nadjieb_mjpeg_streamer::nadjieb_mjpeg_streamer)
