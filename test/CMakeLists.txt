cmake_minimum_required(VERSION 3.28.0)
project(FloppySerialTest)

add_executable(DosFSSerialTest
    DosFSSerialTest.cpp
)
target_link_libraries(DosFSSerialTest PUBLIC FloppySerial DosFS)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(DosFSSerialTest PUBLIC "-fsanitize=undefined")
    target_link_options(DosFSSerialTest PUBLIC "-lubsan" "-fsanitize=undefined")
endif()

add_executable(FuseFunctionsTest
    FuseFunctionsTest.cpp
)

target_link_libraries(FuseFunctionsTest PUBLIC FloppySerial DosFS)
target_compile_definitions(FuseFunctionsTest PUBLIC "-D_FILE_OFFSET_BITS=64")

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(FuseFunctionsTest PUBLIC "-fsanitize=undefined")
    target_link_options(FuseFunctionsTest PUBLIC "-lubsan" "-fsanitize=undefined")
endif()
