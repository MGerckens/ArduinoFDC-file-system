#ifndef FUSEFUNTIONS_H_
#define FUSEFUNTIONS_H_

#include "Helpers.h"
#include "libs/dosfs.h"
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
int serial_mkdir(const char *path, mode_t mode) {}
int serial_rmdir(const char *path) {}
int serial_unlink(const char *path) {}
int serial_read(const char *path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi) {
    if (!data.openedEntities.contains(std::string(path))) {
        return -ENOENT;
    }

    const PFILEINFO file =
        std::get_if<FILEINFO>(&data.openedEntities[std::string(path)]);
    if (file == nullptr) {
        return -EISDIR;
    }

    uint32_t bytesRead = 0;
    uint32_t rc =
        DFS_ReadFile(file, &data.scratchSector[0],
                     reinterpret_cast<uint8_t *>(buf), &bytesRead, size);
    if (rc != DFS_OK && rc != DFS_EOF) {
        return -dfsErrorToErrno(rc);
    }
    return bytesRead;
}
int serial_write(const char *path, const char *buf, size_t size, off_t offset,
                 struct fuse_file_info *fi) {
    if (!data.openedEntities.contains(std::string(path))) {
        return -ENOENT;
    }

    const PFILEINFO file =
        std::get_if<FILEINFO>(&data.openedEntities[std::string(path)]);
    if (file == nullptr) {
        return -EISDIR;
    }

    uint32_t bytesWritten = 0;
    int rc = DFS_WriteFile(file, &data.scratchSector[0],
                           (uint8_t *)(buf + offset), &bytesWritten, size);
    if (rc != DFS_OK && rc != DFS_EOF) {
        return -dfsErrorToErrno(rc);
    }
    return bytesWritten;
}

int serial_mknod(const char *path, mode_t mode, dev_t dev) {}

// needed but lower priority
int serial_getattr(const char *path, struct stat *statbuf) {}
int serial_rename(const char *path, const char *newpath) {}
int serial_statfs(const char *path, struct statvfs *statv) {}
int serial_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi) {}

// maybe needed for dosfs reasons
void *serial_init(struct fuse_conn_info *conn) {}
void serial_destroy(void *userdata) {}
int serial_open(const char *path, struct fuse_file_info *fi) {
    UserData *pData =
        reinterpret_cast<UserData *>(fuse_get_context()->private_data);
    uint8_t mode = unixFlagsToDosMode(fi->flags);
    FILEINFO openedFile{};

    // cast const char* to uint8_t* feels bad, function doesn't seem to modify
    // it
    int rc = DFS_OpenFile(&pData->volInfo, (uint8_t *)path, mode,
                          &pData->scratchSector[0], &openedFile);
    switch (rc) {
    case DFS_OK: {
        data.openedEntities[std::string(path)] = std::move(openedFile);
        return 0;
    }
    case DFS_NOTFOUND:
        return -ENOENT;
    case DFS_WRITEPROT:
        return -EACCES;
    default: {
        std::cerr << "Unhandled return code in serial_open: " << rc << "\n";
        return -EINVAL;
    }
    }
}

// probably not needed
int serial_flush(const char *path, struct fuse_file_info *fi) {}
int serial_release(const char *path, struct fuse_file_info *fi) {}
int serial_fsync(const char *path, int datasync, struct fuse_file_info *fi) {}
int serial_opendir(const char *path, struct fuse_file_info *fi) {}
int serial_releasedir(const char *path, struct fuse_file_info *fi) {}
int serial_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi) {
}
int serial_truncate(const char *path, off_t newsize) {}

// probably impossible
int serial_setxattr(const char *path, const char *name, const char *value,
                    size_t size, int flags) {}
int serial_getxattr(const char *path, const char *name, char *value,
                    size_t size) {}
int serial_listxattr(const char *path, char *list, size_t size) {}
int serial_removexattr(const char *path, const char *name) {}

// definitely impossible
//  int serial_symlink(const char *path, const char *link){}
//  int serial_link(const char *path, const char *newpath){}
//  int serial_readlink(const char *path, char *link, size_t size){}
//  int serial_access(const char *path, int mask){}
//  int serial_chmod(const char *path, mode_t mode){}
//  int serial_chown(const char *path, uid_t uid, gid_t gid){}

#endif
}