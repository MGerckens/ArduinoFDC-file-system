cmake_minimum_required(VERSION 3.28.0)
project(FloppySerial LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    libfuse
    GIT_REPOSITORY "https://github.com/Smit-tay/libfuse-cmake"
    GIT_TAG "fuse-3.15.1"
)
FetchContent_MakeAvailable(libfuse)

# set(INTERFACE_FILE_INCLUDE "\"${CMAKE_CURRENT_SOURCE_DIR}/libs/SerialInterface.h\"")
FetchContent_Declare(
    dosfs
    GIT_REPOSITORY "git@github.com:MGerckens/DosFS.git"
    GIT_TAG "main"
)
FetchContent_MakeAvailable(dosfs)

# dosfs has a very annoying way of defining callbacks
add_library(libs
    libs/SerialInterface.cpp
    libs/SerialInterface.hpp
    libs/SerialWrapper.hpp
    libs/ceserial.hpp
)

SetupDosFS(${CMAKE_CURRENT_SOURCE_DIR}/libs/SerialInterface.hpp libs)

# need to link DosFS as C++ to use SerialInterface (in c++) in the callbacks
set_target_properties(DosFS PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(libs PUBLIC ".")

add_executable(test2
    test2.cpp
)

add_executable(test
    test.cpp
)

add_executable(main
    main.cpp
    FuseFunctions.hpp
    Helpers.hpp
)

target_link_libraries(test2 PUBLIC libs DosFS)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(test2 PUBLIC "-fsanitize=undefined")
    target_link_options(test2 PUBLIC "-lubsan" "-fsanitize=undefined")
endif()

target_link_libraries(test PUBLIC libs)

target_link_libraries(main PUBLIC libs DosFS libfuse.a)
target_compile_definitions(main PUBLIC "-D_FILE_OFFSET_BITS=64")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(main PUBLIC "-fsanitize=undefined")
    target_link_options(main PUBLIC "-lubsan" "-fsanitize=undefined")
endif()
