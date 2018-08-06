cmake_minimum_required(VERSION 3.8)
project(firmwareDaemon)

set(CMAKE_CXX_STANDARD 98)

set(SOURCE_FILES main.cpp unixIPC.cpp)
add_executable(firmwareDaemon ${SOURCE_FILES})