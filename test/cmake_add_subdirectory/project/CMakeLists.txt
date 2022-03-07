cmake_minimum_required(VERSION 3.8)

project(DummyImport CXX)

set(NADJIEB_MJPEG_STREAMER_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(${nadjieb_mjpeg_streamer_source}
   ${CMAKE_CURRENT_BINARY_DIR}/nadjieb_mjpeg_streamer)

add_executable(with_namespace_target main.cpp)
target_link_libraries(with_namespace_target nadjieb_mjpeg_streamer::nadjieb_mjpeg_streamer)

add_executable(without_namespace_target main.cpp)
target_link_libraries(without_namespace_target nadjieb_mjpeg_streamer)
