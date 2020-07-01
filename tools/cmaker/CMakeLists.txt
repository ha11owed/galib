cmake_minimum_required(VERSION 3.1)

project(CMaker)
add_executable(${PROJECT_NAME} "main.cpp"
    "CMaker.h" "CMaker.cpp"
    "../../file_system.h" "../../file_system.cpp"
    "../../process.h" "../../process.cpp" "../../subprocess.h"
    "whereami.h" "whereami.c"
    "tinyxml2.h" "tinyxml2.cpp")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads ${X11_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ../..)

if(CMAKE_DL_LIBS)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
