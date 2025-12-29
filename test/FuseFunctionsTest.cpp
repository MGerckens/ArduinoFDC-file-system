#include "FuseFunctions.hpp"
#include "SerialInterface.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <cstring> // for strerror, strncmp
#include <iostream>
#include <unordered_set>

std::unordered_set<std::string> foundNames;
int setFiller(void *buf, const char *name, const struct stat *stbuf,
              off_t off) {
    foundNames.insert(std::string(name, 11));
    std::cout << name << "\n";
    return 0;
}

void checkDirContains(const char *dirPath,
                      std::unordered_set<std::string> namesToCheck) {
    foundNames.clear();
    int rc = serial_readdir(dirPath, nullptr, setFiller, 0, nullptr);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);
    foundNames.erase(".          ");
    foundNames.erase("..         ");
    EXPECT_EQ(namesToCheck, foundNames);
}

class FuseFunctionFileTest : public ::testing::Test {
  public:
    void SetUp() override {
        fuse_conn_info dummyConnInfo{};
        serial_init(&dummyConnInfo);

        int rc = serial_mknod("testfile.txt", S_IFREG, 0);
        ASSERT_EQ(rc, 0) << std::strerror(-rc);
    }

    void TearDown() override {
        int rc = serial_unlink("testfile.txt");
        ASSERT_EQ(rc, 0) << std::strerror(-rc);
    }
};

TEST_F(FuseFunctionFileTest, MknodUnlinkTest) {
    int rc = serial_mknod("testfile.txt", S_IFREG, 0);
    ASSERT_EQ(rc, -EEXIST);
}

TEST_F(FuseFunctionFileTest, OpenWriteReadTest) {
    fuse_file_info dummyFileInfo{};
    dummyFileInfo.flags |= O_RDWR;
    int result = serial_open("testfile.txt", &dummyFileInfo);
    ASSERT_EQ(result, 0) << std::strerror(-result);

    const char testText[] = "This is a test string";
    constexpr size_t numBytes = sizeof(testText);
    result =
        serial_write("testfile.txt", testText, numBytes, 0, &dummyFileInfo);
    ASSERT_EQ(result, numBytes)
        << ((result < 0) ? std::strerror(-result)
                         : "Incorrect num bytes written");

    char readResult[numBytes];
    result =
        serial_read("testfile.txt", readResult, numBytes, 0, &dummyFileInfo);
    ASSERT_EQ(result, numBytes)
        << ((result < 0) ? std::strerror(-result) : "Incorrect num bytes read");
    ASSERT_STREQ(testText, readResult);
}

TEST_F(FuseFunctionFileTest, AccessTest) {
    int rc = serial_access("testfile.txt", R_OK);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);
}

TEST_F(FuseFunctionFileTest, TruncGetattrTest) {
    fuse_file_info dummyFileInfo{};
    dummyFileInfo.flags |= O_RDWR;
    int result = serial_open("testfile.txt", &dummyFileInfo);
    ASSERT_EQ(result, 0) << std::strerror(-result);
    
    result = serial_truncate("testfile.txt", 513);
    ASSERT_EQ(result, 0) << std::strerror(-result);

    result = serial_fsync("testfile.txt", true, &dummyFileInfo);
    ASSERT_EQ(result, 0) << std::strerror(-result);

    struct stat attrs;
    result = serial_getattr("testfile.txt", &attrs);
    ASSERT_EQ(result, 0) << std::strerror(-result);
    EXPECT_EQ(attrs.st_size, 513);

    result = serial_truncate("testfile.txt", 12);
    ASSERT_EQ(result, 0) << std::strerror(-result);

    result = serial_fsync("testfile.txt", true, &dummyFileInfo);
    ASSERT_EQ(result, 0) << std::strerror(-result);

    result = serial_getattr("testfile.txt", &attrs);
    ASSERT_EQ(result, 0) << std::strerror(-result);
    EXPECT_EQ(attrs.st_size, 12);
}

TEST(FuseFunctionTest, RenameFileWithinFolderTest) {
    fuse_conn_info dummyConnInfo{};
    serial_init(&dummyConnInfo);

    int rc = serial_mknod("testfile.txt", S_IFREG, 0);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    fuse_file_info dummyFileInfo{};
    dummyFileInfo.flags |= O_RDONLY;
    rc = serial_open("testfile.txt", &dummyFileInfo);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    const char testText[] = "This is a test string";
    constexpr size_t numBytes = sizeof(testText);
    rc = serial_write("testfile.txt", testText, numBytes, 0, &dummyFileInfo);
    ASSERT_EQ(rc, numBytes)
        << ((rc < 0) ? std::strerror(-rc) : "Incorrect num bytes written");

    rc = serial_rename("testfile.txt", "newname.txt");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    char readResult[numBytes];
    rc = serial_read("newname.txt", readResult, numBytes, 0, &dummyFileInfo);
    ASSERT_EQ(rc, numBytes)
        << ((rc < 0) ? std::strerror(-rc) : "Incorrect num bytes read");
    ASSERT_STREQ(testText, readResult);

    rc = serial_unlink("newname.txt");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);
}

TEST(FuseFunctionTest, RenameFileFromOneFolderToAnotherTest) {
    fuse_conn_info dummyConnInfo{};
    serial_init(&dummyConnInfo);

    int rc = serial_mknod("testfile.txt", S_IFREG, 0);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    rc = serial_mkdir("folder", S_IFREG);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    fuse_file_info dummyFileInfo{};
    dummyFileInfo.flags |= O_RDONLY;
    rc = serial_open("testfile.txt", &dummyFileInfo);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    const char testText[] = "This is a test string";
    constexpr size_t numBytes = sizeof(testText);
    rc = serial_write("testfile.txt", testText, numBytes, 0, &dummyFileInfo);
    ASSERT_EQ(rc, numBytes)
        << ((rc < 0) ? std::strerror(-rc) : "Incorrect num bytes written");

    rc = serial_rename("testfile.txt", "folder/newname.txt");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    char readResult[numBytes];
    rc = serial_read("folder/newname.txt", readResult, numBytes, 0,
                     &dummyFileInfo);
    ASSERT_EQ(rc, numBytes)
        << ((rc < 0) ? std::strerror(-rc) : "Incorrect num bytes read");
    ASSERT_STREQ(testText, readResult);

    rc = serial_unlink("folder/newname.txt");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    rc = serial_rmdir("folder");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);
    checkDirContains("", {});
}

TEST(FuseFunctionTest, CreateReadDeleteDirectoryTest) {
    fuse_conn_info dummyConnInfo{};
    serial_init(&dummyConnInfo);

    int rc = serial_mkdir("testfold", S_IFREG);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    rc = serial_mknod("testfold/file1.txt", S_IFREG, 0);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    rc = serial_mknod("testfold/file2.txt", S_IFREG, 0);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    checkDirContains("testfold", {"FILE1   TXT", "FILE2   TXT"});

    rc = serial_unlink("testfold/file1.txt");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    rc = serial_unlink("testfold/file2.txt");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    rc = serial_rmdir("testfold");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);
    checkDirContains("", {});
}

TEST(FuseFunctionTest, RenameDirectoryTest) {
    fuse_conn_info dummyConnInfo{};
    serial_init(&dummyConnInfo);

    int rc = serial_mkdir("testfold", S_IFREG);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    rc = serial_mkdir("testfold/subfold", S_IFREG);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    rc = serial_mknod("testfold/subfold/file1.txt", S_IFREG, 0);
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    rc = serial_rename("testfold/subfold", "newfold");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    checkDirContains("", {"TESTFOLD   ", "NEWFOLD    "});
    checkDirContains("testfold", {});
    checkDirContains("newfold", {"FILE1   TXT"});

    rc = serial_unlink("newfold/file1.txt");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    rc = serial_rmdir("testfold");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);

    rc = serial_rmdir("newfold");
    ASSERT_EQ(rc, 0) << std::strerror(-rc);
    checkDirContains("", {});
}
