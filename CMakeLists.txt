cmake_minimum_required(VERSION 3.28.0)
project(FloppySerial CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

# dosfs has a very annoying way of defining callbacks
add_library(libs
            libs/SerialInterface.cpp
            libs/SerialInterface.h
            libs/dosfs.c
            libs/dosfs.h
            libs/ceserial.h
)
# can't link unless dosfs is compiled as C++ instead of C, even with extern "C"
# probably UB somehow
set_source_files_properties("libs/dosfs.c" PROPERTIES LANGUAGE CXX)
target_include_directories(libs PUBLIC ".")

add_executable(test2
               test2.cpp
)

add_executable(test
                test.cpp
) 

add_executable(main
                main.cpp
)           

target_link_libraries(test2 PUBLIC libs)
target_link_libraries(test PUBLIC libs)

