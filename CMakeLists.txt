cmake_minimum_required(VERSION 3.8)
project(OBD)

set(CMAKE_CXX_STANDARD 98)

set(SOURCE_FILES main.cpp ttySetting.cpp parseJ1939.cpp
    canBuffer.hpp formula1939.hpp logger.hpp reportJ1939.hpp)
        
add_executable(OBD ${SOURCE_FILES})
