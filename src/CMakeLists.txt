cmake_minimum_required(VERSION 3.28.0)
project(FloppySerial)

# only used on windows
# see https://gitlab.kitware.com/cmake/cmake/-/issues/26220
FetchContent_Declare(
    ceSerial
    GIT_REPOSITORY "git@github.com:yan9a/serial.git"
    SOURCE_SUBDIR "NOT_A_REAL_SUBDIR"
)
FetchContent_MakeAvailable(ceSerial)

add_library(FloppySerial
    SerialInterface.cpp
    SerialInterface.hpp
    SerialWrapper.hpp
)
target_include_directories(FloppySerial SYSTEM PRIVATE ${FETCHCONTENT_BASE_DIR}/ceserial-src)
target_include_directories(FloppySerial PUBLIC ".")

add_executable(main
    main.cpp
)
target_link_libraries(main PUBLIC FloppySerial DosFS libfuse.a)
target_compile_definitions(main PUBLIC "-D_FILE_OFFSET_BITS=64")

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(main PUBLIC "-fsanitize=undefined")
    target_link_options(main PUBLIC "-lubsan" "-fsanitize=undefined")
endif()
