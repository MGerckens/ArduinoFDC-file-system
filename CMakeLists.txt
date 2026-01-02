cmake_minimum_required(VERSION 3.28.0)
project(FloppySerialBase LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

set(BUILD_FLOPPY_SERIAL_TESTS FALSE CACHE BOOL "Build tests")

add_subdirectory(src)

if(BUILD_FLOPPY_SERIAL_TESTS)
    include(CTest)
    add_subdirectory(test)
endif()
