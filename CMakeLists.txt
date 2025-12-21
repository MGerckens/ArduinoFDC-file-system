cmake_minimum_required(VERSION 3.28.0)
project(FloppySerialBase LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

set(BUILD_FLOPPY_SERIAL_TESTS FALSE CACHE BOOL "Build tests")

include(FetchContent)
FetchContent_Declare(
    libfuse
    GIT_REPOSITORY "git@github.com:Smit-tay/libfuse-cmake.git"
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

add_subdirectory(src)

if(BUILD_FLOPPY_SERIAL_TESTS)
    include(CTest)
    add_subdirectory(test)
endif()
