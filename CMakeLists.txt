cmake_minimum_required(VERSION 3.28.0)
project(FloppySerialBase LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

set(BUILD_FLOPPY_SERIAL_TESTS FALSE CACHE BOOL "Build tests")

include(FetchContent)
FetchContent_Declare(
    libfuse
    GIT_REPOSITORY "https://github.com/Smit-tay/libfuse-cmake"
    GIT_TAG "fuse-3.15.1"
)
FetchContent_MakeAvailable(libfuse)

FetchContent_Declare(
    dosfs
    GIT_REPOSITORY "git@github.com:MGerckens/DosFS.git"
    GIT_TAG "main"
)
FetchContent_MakeAvailable(dosfs)

# need to link DosFS as C++ to use SerialInterface (in c++) in the callbacks
set_target_properties(DosFS PROPERTIES LINKER_LANGUAGE CXX)

add_library(libs
    libs/ceserial.hpp
)
set_target_properties(libs PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(libs PUBLIC ".")

add_subdirectory(src)

add_executable(main
    main.cpp
)
target_link_libraries(main PUBLIC FloppySerial libs DosFS libfuse.a)
target_compile_definitions(main PUBLIC "-D_FILE_OFFSET_BITS=64")

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(main PUBLIC "-fsanitize=undefined")
    target_link_options(main PUBLIC "-lubsan" "-fsanitize=undefined")
endif()

if(BUILD_FLOPPY_SERIAL_TESTS)
    add_subdirectory(test)
endif()
