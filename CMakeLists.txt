cmake_minimum_required(VERSION 3.13)
project(redis-starter-cpp)
set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

set(SOURCE_FILES src/Server.cpp)

add_executable(server ${SOURCE_FILES})

find_package(Threads REQUIRED)
target_link_libraries(mytarget PRIVATE Threads::Threads)