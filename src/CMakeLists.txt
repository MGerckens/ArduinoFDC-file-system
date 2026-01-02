cmake_minimum_required(VERSION 3.28.0)
project(FloppySerial)
include(FetchContent)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # only used on windows
    # see https://gitlab.kitware.com/cmake/cmake/-/issues/26220
    FetchContent_Declare(
        ceSerial
        GIT_REPOSITORY "git@github.com:yan9a/serial.git"
        SOURCE_SUBDIR "NOT_A_REAL_SUBDIR"
    )
    FetchContent_MakeAvailable(ceSerial)

    add_library(WindowsSerial INTERFACE)
    target_include_directories(WindowsSerial SYSTEM INTERFACE "${ceserial_SOURCE_DIR}")

    set(WINFSP_PATH "C:/Program\ Files\ \(x86\)/WinFsp"
        CACHE FILEPATH
        "Path to WinFSP, usually in Program Files (x86)")

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)")
        set(WINFSP_PLATFORM "x86")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(amd64)|(AMD64)")
        set(WINFSP_PLATFORM "x64")
    else()
        set(WINFSP_PLATFORM "a64")
    endif()

    list(APPEND CMAKE_PROGRAM_PATH "${WINFSP_PATH}/bin")
    add_library(FuseImpl INTERFACE)
    target_link_libraries(FuseImpl INTERFACE "${WINFSP_PATH}/lib/winfsp-${WINFSP_PLATFORM}.lib")
else() # Linux
    include(FetchContent)
    FetchContent_Declare(
        libfuse
        GIT_REPOSITORY "git@github.com:Smit-tay/libfuse-cmake.git"
        GIT_TAG "fuse-3.15.1"
    )
    FetchContent_MakeAvailable(libfuse)
    add_library(FuseImpl INTERFACE)
    target_link_libraries(FuseImpl INTERFACE libfuse.a)
    target_compile_definitions(FuseImpl INTERFACE "-D_FILE_OFFSET_BITS=64")
endif()

add_library(FloppySerial
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
    FuseFunctions.cpp
    FuseFunctions.hpp
    FuseWrapper.hpp
)
target_include_directories(FloppySerial PUBLIC ".")
target_link_libraries(FloppySerial PUBLIC FuseImpl)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    include_directories("${WINFSP_PATH}/inc") # ??? why is this needed ???
    target_link_libraries(FloppySerial PUBLIC WindowsSerial)
endif()

add_executable(FatFSMain
    elm-fatfs/main.c
)
target_link_libraries(FatFSMain PUBLIC FloppySerial)

add_executable(main
    main.cpp
)
target_link_libraries(main PUBLIC FloppySerial)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(main PUBLIC "-fsanitize=undefined")
    target_link_options(main PUBLIC "-lubsan" "-fsanitize=undefined")
endif()
