cmake_minimum_required(VERSION 3.8)
project(OBD)

set(CMAKE_CXX_STANDARD 98)

set(SOURCE_FILES main.cpp ttySetting.cpp)
add_executable(OBD ${SOURCE_FILES})