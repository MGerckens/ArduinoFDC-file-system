cmake_minimum_required(VERSION 3.28.0)
project(FloppySerial)

add_library(FloppySerial
    SerialInterface.cpp
    SerialInterface.hpp
    SerialWrapper.hpp
)
target_include_directories(FloppySerial PRIVATE libs)
target_include_directories(FloppySerial PUBLIC ".")
