#include "FuseFunctions.hpp"
#include "SerialInterface.hpp"
#include "SerialWrapper.hpp" // for DEFAULT_PORT
#include "gtest/gtest.h"

#include <cstdint>
#include <iostream>

class FuseFunctionTest : public ::testing::Test {
  public:
    void SetUp() override {
        fuse_conn_info dummyConnInfo{};
        serial_init(&dummyConnInfo);

        int rc = serial_mknod("testfile.txt", S_IFREG, 0);
        ASSERT_EQ(rc, 0);
    }

    void TearDown() override {
        int rc = serial_unlink("testfile.txt");
        ASSERT_EQ(rc, 0);
    }
};

TEST_F(FuseFunctionTest, MknodUnlinkTest) {
    int rc = serial_mknod("testfile.txt", S_IFREG, 0);
    ASSERT_EQ(rc, -EEXIST);
}

TEST_F(FuseFunctionTest, OpenWriteReadTest) {
    fuse_file_info dummyFileInfo{};
    dummyFileInfo.flags |= O_RDONLY;
    int result = serial_open("testfile.txt", &dummyFileInfo);
    ASSERT_EQ(result, 0);

    const char testText[] = "This is a test string";
    constexpr size_t numBytes = sizeof(testText);
    result =
        serial_write("testfile.txt", testText, numBytes, 0, &dummyFileInfo);
    ASSERT_EQ(result, numBytes);

    char readResult[numBytes];
    result =
        serial_read("testfile.txt", readResult, numBytes, 0, &dummyFileInfo);
    ASSERT_EQ(result, numBytes);
    ASSERT_STREQ(testText, readResult);
}

TEST_F(FuseFunctionTest, AccessTest) {
    int rc = serial_access("testfile.txt", R_OK);
    ASSERT_EQ(rc, 0);
}
