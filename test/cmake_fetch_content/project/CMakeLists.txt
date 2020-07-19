cmake_minimum_required(VERSION 3.11)

project(DummyImport CXX)

include(FetchContent)

get_filename_component(GIT_REPOSITORY_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../.. ABSOLUTE)
FetchContent_Declare(nadjieb_mjpeg_streamer GIT_REPOSITORY ${GIT_REPOSITORY_DIRECTORY} GIT_TAG HEAD)

FetchContent_GetProperties(nadjieb_mjpeg_streamer)
if(NOT nadjieb_mjpeg_streamer_POPULATED)
  FetchContent_Populate(nadjieb_mjpeg_streamer)
  add_subdirectory(${nadjieb_mjpeg_streamer_SOURCE_DIR} ${nadjieb_mjpeg_streamer_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_executable(with_namespace_target main.cpp)
target_link_libraries(with_namespace_target nadjieb_mjpeg_streamer::nadjieb_mjpeg_streamer)

add_executable(without_namespace_target main.cpp)
target_link_libraries(without_namespace_target nadjieb_mjpeg_streamer)
