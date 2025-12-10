#ifndef FUSEFUNTIONS_H_
#define FUSEFUNTIONS_H_

#include "Helpers.hpp"
extern "C" {
#include "dosfs.h"
}
#include <cerrno>
#include <fuse.h>
#include <iostream>
#include <memory>
#include <utility>
#include <variant>

extern "C" {
#define FUSE_USE_VERSION 26
#include "fuse.h"

// definitely needed
int serial_mkdir(const char *path, mode_t mode) {
    std::cout << "mkdir\n";

    // ?
    FILEINFO temp;
    uint32_t rc = DFS_OpenFile(&data.volInfo, (uint8_t *)path, DFS_CREATEDIR,
                               data.scratch, &temp);
    return -dfsErrorToErrno(rc);
}

int serial_rmdir(const char *path) {
    // ?
    std::cout << "rmdir\n";
    return 0;
}

int serial_unlink(const char *path) {
    uint32_t rc = DFS_UnlinkFile(&data.volInfo, (uint8_t *)path, data.scratch);
    return -dfsErrorToErrno(rc);
}

int serial_read(const char *path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi) {

    FILEINFO file;
    if (uint32_t rc = DFS_OpenFile(&data.volInfo, (uint8_t *)path, DFS_READ,
                                   data.scratch, &file)) {
        return -dfsErrorToErrno(rc);
    }

    uint32_t bytesRead = 0;
    DFS_Seek(&file, offset, data.scratch);

    if (uint32_t rc =
            DFS_ReadFile(&file, data.scratch, reinterpret_cast<uint8_t *>(buf),
                         &bytesRead, size);
        rc != DFS_OK && rc != DFS_EOF) {
        // DFS_Seek(file, 0, data.scratch);
        return -dfsErrorToErrno(rc);
    }
    return bytesRead;
}

int serial_write(const char *path, const char *buf, size_t size, off_t offset,
                 struct fuse_file_info *fi) {

    FILEINFO file;
    if (uint32_t rc = DFS_OpenFile(&data.volInfo, (uint8_t *)path, DFS_WRITE,
                                   data.scratch, &file)) {
        return -dfsErrorToErrno(rc);
    }

    uint32_t bytesWritten = 0;
    DFS_Seek(&file, offset, data.scratch);
    if (uint32_t rc = DFS_WriteFile(&file, data.scratch, (uint8_t *)buf,
                                    &bytesWritten, size);
        rc != DFS_OK && rc != DFS_EOF) {
        // DFS_Seek(file, 0, data.scratch);
        return -dfsErrorToErrno(rc);
    }
    return bytesWritten;
}

int serial_mknod(const char *path, mode_t mode, dev_t dev) {
    (void)dev; // don't allow creating special files
    if (mode != S_IFREG) {
        return -ENOSYS;
    }

    FILEINFO createdFile{};
    uint32_t rc = DFS_OpenFile(&data.volInfo, (uint8_t *)path, DFS_WRITE,
                               data.scratch, &createdFile);
    return -dfsErrorToErrno(rc);
}

// needed but lower priority
int serial_getattr(const char *path, struct stat *statbuf) {
    std::cout << "getattr\n";
    return 0;
}

int serial_rename(const char *path, const char *newpath) {
    std::cout << "rename\n";
    return 0;
}

int serial_statfs(const char *path, struct statvfs *statv) {
    (void)path;
    std::cout << "statfs\n";
    statv->f_type = 0x65735546; // fuse_super_magic
    statv->f_flag |= ST_SYNCHRONOUS;
    statv->f_bsize = 512;
    statv->f_frsize = 512 * data.volInfo.secperclus;
    statv->f_blocks = data.volInfo.numclusters;
    // todo complete

    return 0;
}

int serial_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi) {
    std::cout << "readdir\n";
    return 0;
}

// maybe needed for dosfs reasons
void *serial_init(struct fuse_conn_info *conn) {
    std::cout << "init\n";
    return nullptr;
}

void serial_destroy(void *userdata) { std::cout << "destroy\n"; }

int serial_open(const char *path, struct fuse_file_info *fi) {
    std::cout << "open\n";
    // because dosfs is stateless, this can be a no op. Just opening a file and
    // doing nothing with it to check if the file exists

    // UserData *pData =
    //     reinterpret_cast<UserData *>(fuse_get_context()->private_data);
    uint8_t mode = unixFlagsToDosMode(fi->flags);
    FILEINFO openedFile{};

    // cast const char* to uint8_t* feels bad, function doesn't seem to modify
    // it
    int rc = DFS_OpenFile(&data.volInfo, (uint8_t *)path, mode, data.scratch,
                          &openedFile);
    if (rc != DFS_OK) {
        return -dfsErrorToErrno(rc);
    }
    // data.openedEntities[std::string(path)] = std::move(openedFile);
    return 0;
}

// probably not needed
int serial_flush(const char *path, struct fuse_file_info *fi) {
    std::cout << "flush\n";
    return 0;
}

int serial_release(const char *path, struct fuse_file_info *fi) {
    std::cout << "release\n";
    return 0;
}

int serial_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
    std::cout << "fsync\n";
    return 0;
}

int serial_opendir(const char *path, struct fuse_file_info *fi) {
    std::cout << "opendir\n";
    return 0;
}

int serial_releasedir(const char *path, struct fuse_file_info *fi) {
    std::cout << "releasedir\n";
    return 0;
}

int serial_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi) {
    std::cout << "fsyncdir\n";
    return 0;
}

int serial_truncate(const char *path, off_t newsize) {
    std::cout << "truncate\n";
    return 0;
}

// probably impossible
int serial_setxattr(const char *path, const char *name, const char *value,
                    size_t size, int flags) {
    std::cout << "setxattr\n";
    return 0;
}

int serial_getxattr(const char *path, const char *name, char *value,
                    size_t size) {
    std::cout << "getxattr\n";
    return 0;
}

int serial_listxattr(const char *path, char *list, size_t size) {
    std::cout << "listxaddr\n";
    return 0;
}

int serial_removexattr(const char *path, const char *name) {
    std::cout << "removexattr\n";
    return 0;
}

// definitely impossible
//  int serial_symlink(const char *path, const char *link){}
//  int serial_link(const char *path, const char *newpath){}
//  int serial_readlink(const char *path, char *link, size_t size){}
//  int serial_access(const char *path, int mask){}
//  int serial_chmod(const char *path, mode_t mode){}
//  int serial_chown(const char *path, uid_t uid, gid_t gid){}

#endif
}