cmake_minimum_required(VERSION 3.28.0)
project(FloppySerialTest)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY git@github.com:google/googletest.git
    GIT_TAG "v1.17.0"
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(FuseFunctionsTest
    FuseFunctionsTest.cpp
)
target_link_libraries(FuseFunctionsTest PUBLIC gtest_main FloppySerial)
gtest_discover_tests(FuseFunctionsTest)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(FuseFunctionsTest PUBLIC "-fsanitize=undefined")
    target_link_options(FuseFunctionsTest PUBLIC "-lubsan" "-fsanitize=undefined")
endif()
