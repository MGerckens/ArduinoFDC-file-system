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
    FuseFunctions.cpp
    FuseFunctions.hpp
    Helpers.cpp
    Helpers.hpp
    SerialInterface.cpp
    SerialWrapper.hpp
    elm-fatfs/diskio.h
    elm-fatfs/ff.c
    elm-fatfs/ff.h
    elm-fatfs/ffconf.h
    elm-fatfs/ffunicode.c
    elm-fatfs/ffsystem.c
)
target_include_directories(FloppySerial SYSTEM PRIVATE ${FETCHCONTENT_BASE_DIR}/ceserial-src)
target_include_directories(FloppySerial PUBLIC ".")
target_compile_definitions(FloppySerial PUBLIC "-D_FILE_OFFSET_BITS=64")

# set_target_properties(FatFS PROPERTIES LINKER_LANGUAGE CXX)
add_executable(FatFSMain
    elm-fatfs/main.c
)
target_link_libraries(FatFSMain PUBLIC FloppySerial)

add_executable(main
    main.cpp
)
target_link_libraries(main PUBLIC FloppySerial libfuse.a)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(main PUBLIC "-fsanitize=undefined")
    target_link_options(main PUBLIC "-lubsan" "-fsanitize=undefined")
endif()
