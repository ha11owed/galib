cmake_minimum_required(VERSION 3.1)

project(CMaker)

add_executable(${PROJECT_NAME} "main.cpp"
    "CMaker.h" "CMaker.cpp"
    # Lib dependencies
    "../../file_system.h" "../../file_system.cpp"
    "../../process.h" "../../process.cpp" "../../subprocess.h"
    "whereami.h" "whereami.c"
    "tinyxml2.h" "tinyxml2.cpp"
    "loguru.hpp" "loguru.cpp"
    # GTest
    "../../gtest/gtest.h"
    "../../gtest/gtest-all.cc")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)
target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../tests>)

if(CMAKE_DL_LIBS)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE CMAKER_WITH_UNIT_TESTS)

file(COPY "tests/testproject_input.cbp" "tests/testproject_output.cbp.xml" "tests/cmaker.json" DESTINATION ${CMAKE_BINARY_DIR})
