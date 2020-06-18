cmake_minimum_required(VERSION 3.1)

project(galib)

add_executable(${PROJECT_NAME}
    "intrusive_containers.h"
    "cache.h"
    "file_system.h" "file_system.cpp"
    "process.h" "process.cpp"
# Tests
    "tests/intrusive_containers_list_tests.cpp"
    "tests/intrusive_containers_hashset_tests.cpp"
    "tests/intrusive_containers_dictionary_tests.cpp"
    "tests/cache_tests.cpp"
    "tests/process_tests.cpp"
    "tests/main.cpp"
# GTest
    "gtest/gtest.h"
    "gtest/gtest-all.cc")

target_include_directories(${PROJECT_NAME} PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_link_libraries(${PROJECT_NAME}
    -pthread
    #X11
    -lstdc++fs -static-libgcc -static-libstdc++)
