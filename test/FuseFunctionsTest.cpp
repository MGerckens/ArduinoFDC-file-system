#define FUSE_USE_VERSION 26
// extern "C" {
// #include "fuse.h"
// }
#include "FuseFunctions.hpp"
#include "SerialInterface.hpp"
#include "SerialWrapper.hpp" // for DEFAULT_PORT

#include <cassert>
#include <cstdint>
#include <iostream>

int main(int argc, char *argv[]) {
    fuse_conn_info dummyConnInfo{};
    serial_init(&dummyConnInfo);

    int rc = serial_mknod("Created.txt", S_IFREG, 0);
    assert(rc == 0);

    rc = serial_mknod("Created.txt", S_IFREG, 0);
    assert(rc == -EEXIST);

    fuse_file_info dummyFileInfo{};
    dummyFileInfo.flags |= O_RDONLY;
    rc = serial_open("Created.txt", &dummyFileInfo);
    assert(rc == 0);

    // const char testText[] = "This is a test string";
    // rc = serial_write("Created.txt", testText, sizeof(testText), 0,
    // &dummyFileInfo); assert(rc == 0);

    rc = serial_unlink("Created.txt");
    assert(rc == 0);

    return 0;
}
