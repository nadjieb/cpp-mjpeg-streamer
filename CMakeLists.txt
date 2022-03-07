cmake_minimum_required(VERSION 3.8)

##
## PROJECT
## name and version
##
project(nadjieb_mjpeg_streamer VERSION 3.0.0 LANGUAGES CXX)

##
## MAIN_PROJECT CHECK
## determine if nadjieb_mjpeg_streamer is built as a subproject (using add_subdirectory) or if it is the main project
##
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()

##
## DEPENDENCIES
##
##
find_package(Threads REQUIRED)

##
## OPTIONS
##

if (POLICY CMP0077)
    # Allow CMake 3.13+ to override options when using FetchContent / add_subdirectory.
    cmake_policy(SET CMP0077 NEW)
endif ()

option(NADJIEB_MJPEG_STREAMER_BuildTests "Build the unit tests when BUILD_TESTING is enabled." ${MAIN_PROJECT})
option(NADJIEB_MJPEG_STREAMER_Install "Install CMake targets during install step." ${MAIN_PROJECT})
option(NADJIEB_MJPEG_STREAMER_MultipleHeaders "Use non-amalgamated version of the library." OFF)

##
## CONFIGURATION
##
include(GNUInstallDirs)

set(NADJIEB_MJPEG_STREAMER_TARGET_NAME                  ${PROJECT_NAME})
set(NADJIEB_MJPEG_STREAMER_CONFIG_INSTALL_DIR           "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE INTERNAL "")
set(NADJIEB_MJPEG_STREAMER_INCLUDE_INSTALL_DIR          "${CMAKE_INSTALL_INCLUDEDIR}")
set(NADJIEB_MJPEG_STREAMER_TARGETS_EXPORT_NAME          "${PROJECT_NAME}Targets")
set(NADJIEB_MJPEG_STREAMER_CMAKE_CONFIG_TEMPLATE        "cmake/config.cmake.in")
set(NADJIEB_MJPEG_STREAMER_CMAKE_CONFIG_DIR             "${CMAKE_CURRENT_BINARY_DIR}")
set(NADJIEB_MJPEG_STREAMER_CMAKE_VERSION_CONFIG_FILE    "${NADJIEB_MJPEG_STREAMER_CMAKE_CONFIG_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(NADJIEB_MJPEG_STREAMER_CMAKE_PROJECT_CONFIG_FILE    "${NADJIEB_MJPEG_STREAMER_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Config.cmake")
set(NADJIEB_MJPEG_STREAMER_CMAKE_PROJECT_TARGETS_FILE   "${NADJIEB_MJPEG_STREAMER_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Targets.cmake")
set(NADJIEB_MJPEG_STREAMER_VERSION_HEADER_TEMPLATE      "${PROJECT_SOURCE_DIR}/cmake/version.hpp.in")

if (NADJIEB_MJPEG_STREAMER_MultipleHeaders)
    set(NADJIEB_MJPEG_STREAMER_INCLUDE_BUILD_DIR "${PROJECT_SOURCE_DIR}/include/")
    message(STATUS "Using the multi-header code from ${NADJIEB_MJPEG_STREAMER_INCLUDE_BUILD_DIR}")
    configure_file(${NADJIEB_MJPEG_STREAMER_VERSION_HEADER_TEMPLATE} "${NADJIEB_MJPEG_STREAMER_INCLUDE_BUILD_DIR}/nadjieb/utils/version.hpp")
else()
    set(NADJIEB_MJPEG_STREAMER_INCLUDE_BUILD_DIR "${PROJECT_SOURCE_DIR}/single_include/")
    message(STATUS "Using the single-header code from ${NADJIEB_MJPEG_STREAMER_INCLUDE_BUILD_DIR}")
endif()

##
## TARGET
## create target and add include path
##
add_library(${NADJIEB_MJPEG_STREAMER_TARGET_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${NADJIEB_MJPEG_STREAMER_TARGET_NAME} ALIAS ${NADJIEB_MJPEG_STREAMER_TARGET_NAME})
if (${CMAKE_VERSION} VERSION_LESS "3.8.0")
    target_compile_features(${NADJIEB_MJPEG_STREAMER_TARGET_NAME} INTERFACE cxx_range_for)
else()
    target_compile_features(${NADJIEB_MJPEG_STREAMER_TARGET_NAME} INTERFACE cxx_std_17)
endif()

target_include_directories(
    ${NADJIEB_MJPEG_STREAMER_TARGET_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${NADJIEB_MJPEG_STREAMER_INCLUDE_BUILD_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    ${NADJIEB_MJPEG_STREAMER_TARGET_NAME}
    INTERFACE
    Threads::Threads
)

# Install a pkg-config file, so other tools can find this.
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pkg-config.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
)

##
## TESTS
## create and configure the unit test target
##
include(CTest) #adds option BUILD_TESTING (default ON)

if(BUILD_TESTING AND NADJIEB_MJPEG_STREAMER_BuildTests)
    enable_testing()
    add_subdirectory(test)
endif()

##
## INSTALL
## install header files, generate and install cmake config files for find_package()
##
include(CMakePackageConfigHelpers)
# use a custom package version config file instead of
# write_basic_package_version_file to ensure that it's architecture-independent
configure_file(
    "cmake/nadjieb_mjpeg_streamerConfigVersion.cmake.in"
    ${NADJIEB_MJPEG_STREAMER_CMAKE_VERSION_CONFIG_FILE}
    @ONLY
)
configure_file(
    ${NADJIEB_MJPEG_STREAMER_CMAKE_CONFIG_TEMPLATE}
    ${NADJIEB_MJPEG_STREAMER_CMAKE_PROJECT_CONFIG_FILE}
    @ONLY
)

if(NADJIEB_MJPEG_STREAMER_Install)
    install(
        DIRECTORY ${NADJIEB_MJPEG_STREAMER_INCLUDE_BUILD_DIR}
        DESTINATION ${NADJIEB_MJPEG_STREAMER_INCLUDE_INSTALL_DIR}
    )
    install(
        FILES ${NADJIEB_MJPEG_STREAMER_CMAKE_PROJECT_CONFIG_FILE} ${NADJIEB_MJPEG_STREAMER_CMAKE_VERSION_CONFIG_FILE}
        DESTINATION ${NADJIEB_MJPEG_STREAMER_CONFIG_INSTALL_DIR}
    )
    export(
        TARGETS ${NADJIEB_MJPEG_STREAMER_TARGET_NAME}
        NAMESPACE ${PROJECT_NAME}::
        FILE ${NADJIEB_MJPEG_STREAMER_CMAKE_PROJECT_TARGETS_FILE}
    )
    install(
        TARGETS ${NADJIEB_MJPEG_STREAMER_TARGET_NAME}
        EXPORT ${NADJIEB_MJPEG_STREAMER_TARGETS_EXPORT_NAME}
        INCLUDES DESTINATION ${NADJIEB_MJPEG_STREAMER_INCLUDE_INSTALL_DIR}
    )
    install(
        EXPORT ${NADJIEB_MJPEG_STREAMER_TARGETS_EXPORT_NAME}
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${NADJIEB_MJPEG_STREAMER_CONFIG_INSTALL_DIR}
    )
    install(
        FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc"
        DESTINATION lib/pkgconfig
    )
endif()
