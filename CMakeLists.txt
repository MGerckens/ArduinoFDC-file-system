cmake_minimum_required(VERSION 3.28.0)
project(FloppySerial C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

add_library(dosfs
           dosfs-1.03/dosfs.cpp
           dosfs-1.03/dosfs.h
           dosfs-1.03/SerialInterface.cpp
           dosfs-1.03/SerialInterface.h
           dosfs-1.03/ceserial.h
)

add_executable(test2
               test2.cpp
)

add_executable(test
                test.cpp
) 

add_executable(main
                main.cpp
)           

target_link_libraries(test2 PUBLIC dosfs)
target_link_libraries(test PUBLIC dosfs)

