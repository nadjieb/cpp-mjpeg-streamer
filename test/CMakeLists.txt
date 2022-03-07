option(NADJIEB_MJPEG_STREAMER_Coverage "Build test suite with coverage information" OFF)

if(NADJIEB_MJPEG_STREAMER_Coverage)
    message(STATUS "Building test suite with coverage information")

    # from https://github.com/RWTH-HPC/CMake-codecov/blob/master/cmake/FindGcov.cmake
    get_filename_component(COMPILER_PATH "${CMAKE_CXX_COMPILER}" PATH)
    string(REGEX MATCH "^[0-9]+" GCC_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
    find_program(GCOV_BIN NAMES gcov-${GCC_VERSION} gcov HINTS ${COMPILER_PATH})

    # collect all source files from the chosen include dir
    file(GLOB_RECURSE SOURCE_FILES ${NADJIEB_MJPEG_STREAMER_INCLUDE_BUILD_DIR}*.hpp)

    # add target to collect coverage information and generate HTML file
    add_custom_target(lcov_html
        COMMAND lcov --directory . --capture --output-file nadjieb_mjpeg_streamer.info
        COMMAND lcov --extract nadjieb_mjpeg_streamer.info ${SOURCE_FILES} --output-file nadjieb_mjpeg_streamer.info.filtered --gcov-tool ${GCOV_BIN}
        COMMAND genhtml --title "C++ MJPEG over HTTP Library" --legend --output-directory html --show-details nadjieb_mjpeg_streamer.info.filtered
        COMMENT "Generating HTML report test/html/index.html"
    )

    add_custom_target(lcov
        COMMAND lcov --directory . --capture --output-file nadjieb_mjpeg_streamer.info
        COMMAND lcov --extract nadjieb_mjpeg_streamer.info ${SOURCE_FILES} --output-file nadjieb_mjpeg_streamer.info.filtered --gcov-tool ${GCOV_BIN}
    )
endif()

#############################################################################
# doctest library with the main function to speed up build
#############################################################################

add_library(doctest_main OBJECT src/unit.cpp)
if (${CMAKE_VERSION} VERSION_LESS "3.8.0")
    target_compile_features(doctest_main PUBLIC cxx_range_for)
else()
    target_compile_features(doctest_main PUBLIC cxx_std_17)
endif()
target_include_directories(doctest_main PRIVATE include)

#############################################################################
# one executable for each unit test file
#############################################################################

set(files
    src/unit-version.cpp
    src/unit-http-request.cpp
    src/unit-http-response.cpp
    src/unit-socket.cpp
    src/unit-listener.cpp
    src/unit-streamer.cpp
)

foreach(file ${files})
    get_filename_component(file_basename ${file} NAME_WE)
    string(REGEX REPLACE "unit-([^$]+)" "test-\\1" testcase ${file_basename})

    add_executable(${testcase} $<TARGET_OBJECTS:doctest_main> ${file})
    target_compile_definitions(${testcase} PRIVATE DOCTEST_CONFIG_SUPER_FAST_ASSERTS)
    target_compile_options(${testcase} PRIVATE
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-deprecated;-Wno-float-equal>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations>
    )
    target_include_directories(${testcase} PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/test/include)
    target_link_libraries(${testcase} PRIVATE ${NADJIEB_MJPEG_STREAMER_TARGET_NAME})

    if (NADJIEB_MJPEG_STREAMER_Coverage)
        target_compile_options(${testcase} PRIVATE --coverage)
        target_link_libraries(${testcase} PRIVATE --coverage)
    endif()

    add_test(NAME "${testcase}"
        COMMAND ${testcase} ${DOCTEST_TEST_FILTER} --no-skip
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties("${testcase}" PROPERTIES LABELS "all" FIXTURES_REQUIRED TEST_DATA)
endforeach()

add_executable(nadjieb_mjpeg_streamer_unit EXCLUDE_FROM_ALL $<TARGET_OBJECTS:doctest_main> ${files})
target_compile_definitions(nadjieb_mjpeg_streamer_unit PRIVATE DOCTEST_CONFIG_SUPER_FAST_ASSERTS)
target_compile_options(nadjieb_mjpeg_streamer_unit PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-deprecated;-Wno-float-equal>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations>
)
target_include_directories(nadjieb_mjpeg_streamer_unit PRIVATE ${CMAKE_BINARY_DIR}/include include)
target_link_libraries(nadjieb_mjpeg_streamer_unit ${NADJIEB_MJPEG_STREAMER_TARGET_NAME})

#############################################################################
# Test the generated build configs
#############################################################################

add_subdirectory(cmake_import)
add_subdirectory(cmake_import_minver)
add_subdirectory(cmake_add_subdirectory)
add_subdirectory(cmake_fetch_content)
add_subdirectory(cmake_target_include_directories)
