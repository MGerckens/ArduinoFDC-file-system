#include "FuseFunctions.hpp"
#include "SerialInterface.hpp"
#include "SerialWrapper.hpp" // for DEFAULT_PORT
#include "gtest/gtest.h"

#include <cstdint>
#include <cstring> // for memcpy
#include <iostream>

class FuseFunctionFileTest : public ::testing::Test {
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

TEST_F(FuseFunctionFileTest, MknodUnlinkTest) {
    int rc = serial_mknod("testfile.txt", S_IFREG, 0);
    ASSERT_EQ(rc, -EEXIST);
}

TEST_F(FuseFunctionFileTest, OpenWriteReadTest) {
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

TEST_F(FuseFunctionFileTest, AccessTest) {
    int rc = serial_access("testfile.txt", R_OK);
    ASSERT_EQ(rc, 0);
}

TEST(FuseFunctionTest, RenameFileTest) {
    fuse_conn_info dummyConnInfo{};
    serial_init(&dummyConnInfo);

    int rc = serial_mknod("testfile.txt", S_IFREG, 0);
    ASSERT_EQ(rc, 0);

    fuse_file_info dummyFileInfo{};
    dummyFileInfo.flags |= O_RDONLY;
    int result = serial_open("testfile.txt", &dummyFileInfo);
    ASSERT_EQ(result, 0);

    const char testText[] = "This is a test string";
    constexpr size_t numBytes = sizeof(testText);
    result =
        serial_write("testfile.txt", testText, numBytes, 0, &dummyFileInfo);
    ASSERT_EQ(result, numBytes);

    // creates the directory entry but loses the data
    result = serial_rename("testfile.txt", "newname.txt");
    ASSERT_EQ(result, 0);

    char readResult[numBytes];
    result =
        serial_read("newname.txt", readResult, numBytes, 0, &dummyFileInfo);
    ASSERT_EQ(result, numBytes);
    ASSERT_STREQ(testText, readResult);

    rc = serial_unlink("newname.txt");
    ASSERT_EQ(rc, 0);
}

static bool gotFile1;
static bool gotFile2;
int dummyFillerFunction(void *buf, const char *name, const struct stat *stbuf,
                        off_t off) {
    std::cout << name << "\n";
    if (!std::strncmp(name, "FILE1   TXT", 12)) {
        gotFile1 = true;
    }
    if (!std::strncmp(name, "FILE2   TXT", 12)) {
        gotFile2 = true;
    }
    return 0;
}

TEST(FuseFunctionDirectoryTest, CreateReadDeleteDirectoryTest) {
    fuse_conn_info dummyConnInfo{};
    serial_init(&dummyConnInfo);

    int rc = serial_mkdir("testfold", S_IFREG);
    ASSERT_EQ(rc, 0);

    rc = serial_mknod("testfold/file1.txt", S_IFREG, 0);
    ASSERT_EQ(rc, 0);

    rc = serial_mknod("testfold/file2.txt", S_IFREG, 0);
    ASSERT_EQ(rc, 0);

    rc = serial_readdir("testfold", nullptr, dummyFillerFunction, 0, nullptr);
    EXPECT_TRUE(gotFile1);
    EXPECT_TRUE(gotFile2);
    ASSERT_EQ(rc, 0);

    rc = serial_unlink("testfold/file1.txt");
    ASSERT_EQ(rc, 0);

    rc = serial_unlink("testfold/file2.txt");
    ASSERT_EQ(rc, 0);

    rc = serial_rmdir("testfold");
    ASSERT_EQ(rc, 0);
}

TEST(FuseFunctionDirectoryTest, RenameDirectoryTest) {
    fuse_conn_info dummyConnInfo{};
    serial_init(&dummyConnInfo);

    int rc = serial_mkdir("testfold", S_IFREG);
    ASSERT_EQ(rc, 0);

    rc = serial_mkdir("testfold/subfold", S_IFREG);
    ASSERT_EQ(rc, 0);

    rc = serial_mknod("testfold/subfold/file1.txt", S_IFREG, 0);
    ASSERT_EQ(rc, 0);

    rc = serial_readdir("testfold", nullptr, dummyFillerFunction, 0, nullptr);
    EXPECT_TRUE(gotFile1);
    EXPECT_TRUE(gotFile2);
    ASSERT_EQ(rc, 0);

    rc = serial_unlink("testfold/file1.txt");
    ASSERT_EQ(rc, 0);

    rc = serial_unlink("testfold/file2.txt");
    ASSERT_EQ(rc, 0);

    rc = serial_rmdir("testfold");
    ASSERT_EQ(rc, 0);
}
