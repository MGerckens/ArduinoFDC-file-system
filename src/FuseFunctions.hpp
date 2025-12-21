#ifndef FUSEFUNCTIONS_H_
#define FUSEFUNCTIONS_H_

#include "Helpers.hpp"
extern "C" {
#include "dosfs.h"
}
#include "SerialInterface.hpp"
#include <algorithm> // for min
#include <array>
#include <cerrno>
#include <cmath> // for ceil
#include <cstdint>
#include <cstring> // for memcpy
#include <iostream>

extern "C" {

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include "fuse.h"

// definitely needed
int serial_mkdir(const char *path, mode_t mode) {
    FILEINFO temp;
    return -DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector,
                         &data.volInfo, path, DFS_CREATEDIR, data.scratch,
                         &temp);
}

int serial_unlink(const char *path) {
    return -DFS_UnlinkFile(DFS_SerialReadSector, DFS_SerialWriteSector,
                           &data.volInfo, path, data.scratch);
}

int serial_rmdir(const char *path) {
    // check that dir is empty
    DIRINFO dir{};
    dir.scratch = data.scratch;
    uint32_t rc = DFS_OpenDir(DFS_SerialReadSector, &data.volInfo, path, &dir);
    if (rc) {
        return -rc;
    }
    DIRENT entry{};
    while (
        (rc = DFS_GetNext(DFS_SerialReadSector, &data.volInfo, &dir, &entry)),
        (rc == DFS_OK)) {
        if (entry.name[0] != 0 && std::strncmp(entry.name, ".          ", 11) &&
            std::strncmp(entry.name, "..         ", 11)) {
            return -ENOTEMPTY;
        }
    }

    if (rc == DFS_EOF) {
        return 0;
    } else {
        return -rc;
    }

    return serial_unlink(path);
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
    return -DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector,
                         &data.volInfo, path, DFS_CREATEFILE, data.scratch,
                         &createdFile);
}

int serial_getattr(const char *path, struct stat *statbuf) {
    FILEINFO file{};
    uint32_t rc =
        DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector, &data.volInfo,
                     path, DFS_READ, data.scratch, &file);
    if (rc) {
        return -rc;
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
    std::cerr << "rename\n";
    return -ENOSYS;
}

int serial_statfs(const char *path, struct statvfs *statv) {
    (void)path;
    std::cerr << "statfs\n";
    statv->f_type = 0x65735546; // fuse_super_magic
    statv->f_flag |= ST_SYNCHRONOUS;
    statv->f_bsize = 512;
    statv->f_frsize = 512 * data.volInfo.secperclus;
    statv->f_blocks = data.volInfo.numclusters;
    // todo complete

    return -ENOSYS;
}

int serial_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi) {
    DIRINFO dir{};
    dir.scratch = data.scratch;
    uint32_t rc = DFS_OpenDir(DFS_SerialReadSector, &data.volInfo, path, &dir);
    if (rc) {
        return -rc;
    }

    int i = 0;
    DIRENT entry{};
    // skip to desired offset
    for (i = 0; i < offset; ++i) {
        if (DFS_GetNext(DFS_SerialReadSector, &data.volInfo, &dir, &entry)) {
            return 0;
        }
    }
    while (
        (rc = DFS_GetNext(DFS_SerialReadSector, &data.volInfo, &dir, &entry)),
        (rc == DFS_OK && entry.name[0] != 0)) {
        // struct stat attributes;
        char nullTerminatedName[12];
        std::memcpy(nullTerminatedName, entry.name, 11);
        nullTerminatedName[11] = '\0';
        if (filler(buf, nullTerminatedName, NULL, offset + 1)) {
            return 0;
        }
    }

    if (rc == DFS_EOF) {
        return 0;
    } else {
        return -rc;
    }
}

void *serial_init(struct fuse_conn_info *conn) {
    InitDosFsSerialConnection(DEFAULT_PORT, 115200);

    data.partitionStart = DFS_GetPtnStart(DFS_SerialReadSector, 0, data.scratch,
                                          0, NULL, NULL, &data.partitionSize);
    uint32_t rc = DFS_GetVolInfo(DFS_SerialReadSector, 0, data.scratch,
                                 data.partitionStart, &data.volInfo);

    conn->max_write = std::min<std::size_t>(UINT32_MAX, SIZE_MAX);
    conn->async_read = 0;
    conn->max_readahead = 0;
    conn->max_background = 0;
    // todo remove FUSE_CAP_ATOMIC_O_TRUNC when trunc is supported
    conn->capable =
        conn->want & ~(FUSE_CAP_ASYNC_READ | FUSE_CAP_ATOMIC_O_TRUNC);

    return nullptr;
}

int serial_open(const char *path, struct fuse_file_info *fi) {
    // because dosfs is stateless, this can be a no op. Just opening a file and
    // doing nothing with it to check if the file exists

    uint8_t mode = unixFlagsToDosMode(fi->flags);
    FILEINFO openedFile{};

    uint32_t rc =
        DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector, &data.volInfo,
                     path, mode, data.scratch, &openedFile);
    fi->direct_io = 1;
    return -rc;
}

int serial_access(const char *path, int mask) {
    FILEINFO dummyFile{};

    return -DFS_OpenFile(DFS_SerialReadSector, DFS_SerialWriteSector,
                         &data.volInfo, path, DFS_READ, data.scratch,
                         &dummyFile);
}

int serial_truncate(const char *path, off_t newsize) {
    std::cerr << "truncate\n";
    return -ENOSYS;
}

int serial_ftruncate(const char *path, off_t newsize, struct fuse_file_info *) {
    return serial_truncate(path, newsize);
}

int serial_opendir(const char *path, struct fuse_file_info *fi) {
    std::cerr << "opendir\n";
    return -ENOSYS;
}

// needs new functionality in dosfs
int serial_utimens(const char *path, const struct timespec tv[2]) {
    std::cerr << "utimens\n";
    return -ENOSYS;
}

// probably can leave as no-ops
// destroy will be useful if I put userData in private_data instead of a global
void serial_destroy(void *userdata) { std::cout << "destroy\n"; }

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

int serial_releasedir(const char *path, struct fuse_file_info *fi) {
    std::cout << "releasedir\n";
    return 0;
}

int serial_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi) {
    std::cout << "fsyncdir\n";
    return 0;
}

// probably impossible
int serial_setxattr(const char *path, const char *name, const char *value,
                    size_t size, int flags) {
    std::cerr << "setxattr\n";
    return -ENOSYS;
}

int serial_getxattr(const char *path, const char *name, char *value,
                    size_t size) {
    std::cerr << "getxattr\n";
    return -ENOSYS;
}

int serial_listxattr(const char *path, char *list, size_t size) {
    std::cerr << "listxaddr\n";
    return -ENOSYS;
}

int serial_removexattr(const char *path, const char *name) {
    std::cerr << "removexattr\n";
    return -ENOSYS;
}

// definitely impossible
int serial_symlink(const char *path, const char *link) {
    std::cerr << "symlink\n";
    return -ENOSYS;
}
int serial_link(const char *path, const char *newpath) {
    std::cerr << "link\n";
    return -ENOSYS;
}
int serial_readlink(const char *path, char *link, size_t size) {
    std::cerr << "readlink\n";
    return -ENOSYS;
}
int serial_chmod(const char *path, mode_t mode) {
    std::cerr << "chmod\n";
    return -ENOSYS;
}
int serial_chown(const char *path, uid_t uid, gid_t gid) {
    std::cerr << "chown\n";
    return -ENOSYS;
}
}
#endif
