#ifndef FUSEFUNCTIONS_H_
#define FUSEFUNCTIONS_H_

#include "Helpers.hpp"
extern "C" {
#include "dosfs.h"
}
#include "SerialInterface.hpp"
#include <algorithm> // for min
#include <cerrno>
#include <cmath> // for ceil
#include <cstdint>
#include <iostream>

extern "C" {

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include "fuse.h"

// definitely needed
int serial_mkdir(const char *path, mode_t mode) {
    std::cout << "mkdir\n";

    // ?
    FILEINFO temp;
    uint32_t rc =
        DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector, &data.volInfo,
                     path, DFS_CREATEDIR, data.scratch, &temp);
    return -rc;
}

int serial_rmdir(const char *path) {
    // ?
    std::cout << "rmdir\n";
    return 0;
}

int serial_unlink(const char *path) {
    uint32_t rc = DFS_UnlinkFile(DFS_SerialReadSector, DFS_SerialWriteSector,
                                 &data.volInfo, path, data.scratch);
    return -rc;
}

int serial_read(const char *path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi) {

    FILEINFO file;
    if (uint32_t rc =
            DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector,
                         &data.volInfo, path, DFS_READ, data.scratch, &file)) {
        return -rc;
    }

    uint32_t bytesRead = 0;
    DFS_Seek(DFS_SerialReadSector, &file, offset, data.scratch);

    if (uint32_t rc =
            DFS_ReadFile(DFS_SerialReadSector, &file, data.scratch,
                         reinterpret_cast<uint8_t *>(buf), &bytesRead, size);
        rc != DFS_OK && rc != DFS_EOF) {
        // DFS_Seek(file, 0, data.scratch);
        return -rc;
    }
    return bytesRead;
}

int serial_write(const char *path, const char *buf, size_t size, off_t offset,
                 struct fuse_file_info *fi) {

    FILEINFO file;
    if (uint32_t rc =
            DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector,
                         &data.volInfo, path, DFS_WRITE, data.scratch, &file)) {
        return -rc;
    }

    uint32_t bytesWritten = 0;
    DFS_Seek(DFS_SerialReadSector, &file, offset, data.scratch);
    if (uint32_t rc = DFS_WriteFile(DFS_SerialReadSector, DFS_SerialWriteSector,
                                    &file, data.scratch, (const uint8_t *)buf,
                                    &bytesWritten, size);
        rc != DFS_OK && rc != DFS_EOF) {
        // DFS_Seek(file, 0, data.scratch);
        return -rc;
    }
    return bytesWritten;
}

int serial_mknod(const char *path, mode_t mode, dev_t dev) {
    (void)dev; // don't allow creating special files
    if ((mode & S_IFMT) != S_IFREG) {
        return -ENOSYS;
    }

    FILEINFO createdFile{};
    uint32_t rc =
        DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector, &data.volInfo,
                     path, DFS_CREATEFILE, data.scratch, &createdFile);
    return -rc;
}

int serial_getattr(const char *path, struct stat *statbuf) {
    FILEINFO file{};
    uint32_t rc =
        DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector, &data.volInfo,
                     path, DFS_READ, data.scratch, &file);
    if (rc) {
        return rc;
    }

    statbuf->st_dev = 0;        // assigned by fuse
    statbuf->st_ino = 0;        // assigned by fuse
    statbuf->st_mode = S_IFREG; // only regular files are supported
    statbuf->st_nlink = 0;      // FAT12 doesn't support hardlinks
    statbuf->st_uid = 0;        // or users
    statbuf->st_gid = 0;        // or groups
    statbuf->st_rdev = 0;       // not a device
    statbuf->st_size = file.filelen;
    statbuf->st_blksize = 512;
    statbuf->st_blocks =
        std::ceil(statbuf->st_size /
                  512.0); // this might be inaccurate if the file is fragmented
    statbuf->st_atim = timespec{0, 0}; // todo: implement timestamps
    statbuf->st_mtim = timespec{0, 0}; // todo: implement timestamps
    statbuf->st_ctim = timespec{0, 0}; // todo: implement timestamps

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

void *serial_init(struct fuse_conn_info *conn) {
    InitDosFsSerialConnection(DEFAULT_PORT, 115200);

    data.partitionStart = DFS_GetPtnStart(DFS_SerialReadSector, 0, data.scratch,
                                          0, NULL, NULL, &data.partitionSize);
    int rc = DFS_GetVolInfo(DFS_SerialReadSector, 0, data.scratch,
                            data.partitionStart, &data.volInfo);

    conn->max_write = std::min<std::size_t>(UINT32_MAX, SIZE_MAX);
    conn->async_read = 0;
    conn->max_readahead = 0;
    conn->max_background = 0;
    conn->capable =
        conn->want & ~(FUSE_CAP_ASYNC_READ | FUSE_CAP_ATOMIC_O_TRUNC);

    return nullptr;
}

void serial_destroy(void *userdata) { std::cout << "destroy\n"; }

int serial_open(const char *path, struct fuse_file_info *fi) {
    // because dosfs is stateless, this can be a no op. Just opening a file and
    // doing nothing with it to check if the file exists

    uint8_t mode = unixFlagsToDosMode(fi->flags);
    FILEINFO openedFile{};

    int rc = DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector,
                          &data.volInfo, path, mode, data.scratch, &openedFile);
    fi->direct_io = 1;
    return -rc;
}

int serial_access(const char *path, int mask) {
    FILEINFO dummyFile{};
    int rc =
        DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector, &data.volInfo,
                     path, DFS_READ, data.scratch, &dummyFile);
    return -rc;
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
//  int serial_chmod(const char *path, mode_t mode){}
//  int serial_chown(const char *path, uid_t uid, gid_t gid){}
}
#endif
