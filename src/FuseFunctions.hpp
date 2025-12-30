#ifndef FUSEFUNCTIONS_H_
#define FUSEFUNCTIONS_H_

#include "Helpers.hpp"
#include "SerialInterface.hpp"
#include <algorithm> // for min
#include <array>
#include <cerrno>
#include <cmath> // for ceil
#include <cstdint>
#include <cstring> // for memcpy
#include <filesystem>
#include <iostream>
#include <memory>

extern "C" {

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include "fuse.h"

FATFS FatFs;
std::unordered_map<std::string, FIL> openFiles;
std::unordered_map<std::string, DIR> openDirs;

// definitely needed
int serial_mkdir(const char *path, mode_t mode) {
    return -fresultToErrno(f_mkdir(path));
}

int serial_unlink(const char *path) {
    auto fileIter = openFiles.find(std::string(path));
    if (fileIter != openFiles.end()) {
        FRESULT rc = f_close(&fileIter->second);
        if (rc != 0) {
            return -fresultToErrno(rc);
        }
        openFiles.erase(fileIter);
    }

    return -fresultToErrno(f_unlink(path));
}

int serial_rmdir(const char *path) {
    auto dirIter = openDirs.find(std::string(path));
    if (dirIter != openDirs.end()) {
        FRESULT rc = f_closedir(&dirIter->second);
        if (rc != 0) {
            return -fresultToErrno(rc);
        }
        openDirs.erase(dirIter);
    }

    return -fresultToErrno(f_unlink(path));
}

int serial_read(const char *path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi) {
    auto fileIter = openFiles.find(std::string(path));
    if (fileIter == openFiles.end()) {
        return -ENOENT;
    }
    UINT bytesRead = 0;
    FRESULT rc = f_lseek(&fileIter->second, offset);
    if (rc) {
        return -fresultToErrno(rc);
    }
    rc = f_read(&fileIter->second, buf, size, &bytesRead);
    return rc ? -fresultToErrno(rc) : bytesRead;
}

int serial_write(const char *path, const char *buf, size_t size, off_t offset,
                 struct fuse_file_info *fi) {
    auto fileIter = openFiles.find(std::string(path));
    if (fileIter == openFiles.end()) {
        return -ENOENT;
    }
    UINT bytesWritten = 0;
    FRESULT rc = f_lseek(&fileIter->second, offset);
    if (rc) {
        return -fresultToErrno(rc);
    }
    rc = f_write(&fileIter->second, buf, size, &bytesWritten);
    return rc ? -fresultToErrno(rc) : bytesWritten;
}

int serial_mknod(const char *path, mode_t mode, dev_t dev) {
    (void)dev; // don't allow creating special files
    if ((mode & S_IFMT) != S_IFREG) {
        std::cerr
            << "mknod: tried to create something other than a regular file\n";
        return -ENOSYS;
    }

    FIL file;
    FRESULT rc = f_open(&file, path, FA_CREATE_NEW);
    if (!rc) {
        f_close(&file);
    }
    return -fresultToErrno(rc);
}

int serial_getattr(const char *path, struct stat *statbuf) {
    FILINFO info;
    FRESULT rc = f_stat(path, &info);
    if (rc) {
        return -fresultToErrno(rc);
    }

    statbuf->st_dev = 0; // assigned by fuse
    statbuf->st_ino = 0; // assigned by fuse
    // give everyone all permissions
    statbuf->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;
    if (info.fattrib & AM_DIR) {
        statbuf->st_mode |= S_IFDIR;
    } else {
        statbuf->st_mode |= S_IFREG;
    }
    statbuf->st_nlink = 0; // FAT12 doesn't support hardlinks
    statbuf->st_uid = 0;   // or users
    statbuf->st_gid = 0;   // or groups
    statbuf->st_rdev = 0;  // not a device
    statbuf->st_size = info.fsize;
    statbuf->st_blksize = 512;
    // this might be inaccurate if the file is fragmented
    statbuf->st_blocks = std::ceil(statbuf->st_size / 512.0);
    statbuf->st_mtim = timespec{fatTimeToSeconds(info.fdate, info.ftime), 0};
    // can't get access time, use modification time
    statbuf->st_atim = statbuf->st_mtim;
    statbuf->st_ctim = timespec{fatTimeToSeconds(info.crdate, info.crtime), 0};

    return 0;
}

int serial_fgetattr(const char *path, struct stat *stbuf,
                    struct fuse_file_info *fi) {
    (void)fi;
    return serial_getattr(path, stbuf);
}

int serial_rename(const char *path, const char *newpath) {
    if (openFiles.contains(std::string(path))) {
        std::cerr << "cannot rename an open file\n";
        return -EINVAL;
    }
    return -fresultToErrno(f_rename(path, newpath));
}

int serial_statfs(const char *path, struct statvfs *statv) {
    (void)path;
    statv->f_bsize = 512 * FatFs.csize;
    statv->f_frsize = statv->f_bsize;
    statv->f_blocks = FatFs.n_fatent - 2;
    statv->f_bfree = FatFs.free_clst;
    statv->f_bavail = statv->f_ffree;

    statv->f_files = 0; // FAT doesn't use inodes
    statv->f_ffree = 0;
    statv->f_favail = 0;

    statv->f_flag = (ST_SYNCHRONOUS | ST_NODEV);
    if (FF_FS_READONLY) {
        statv->f_flag |= ST_RDONLY;
    }
    statv->f_type = 0x65735546; // fuse_super_magic
    statv->f_namemax = 99;
    return 0;
}

int serial_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi) {
    auto dirIter = openDirs.find(std::string(path));
    if (dirIter == openDirs.end()) {
        return -ENOENT;
    }

    FILINFO info;
    int i = 0;
    for (i = 0; i < offset; ++i) {
        f_readdir(&dirIter->second, &info);
        if (info.fname[0] == '\x00') {
            return 0;
        }
    }
    while (!f_readdir(&dirIter->second, &info)) {
        if (info.fname[0] == '\x00') {
            return 0;
        }
        if (filler(buf, info.fname, NULL, i++)) {
            return 0;
        }
    }
    return 0;
}

void *serial_init(struct fuse_conn_info *conn) {
    f_mount(&FatFs, "", 0);

    conn->max_write = 0;
    conn->async_read = 0;
    conn->max_readahead = 0;
    conn->max_background = 0;
    conn->want |= FUSE_CAP_BIG_WRITES;

    return nullptr;
}

int serial_open(const char *path, struct fuse_file_info *fi) {
    fi->direct_io = 1;
    // for ??reasons??, opening a FIL and then adding it to the map causes it to
    // be corrupted inserting an empty FIL and then opening it works
    openFiles.insert({std::string(path), FIL{}});
    FRESULT rc = f_open(&openFiles.at(std::string(path)), path,
                        unixFlagsToFatFlags(fi->flags));
    if (rc) {
        openFiles.erase(std::string(path));
    }

    return -fresultToErrno(rc);
}

int serial_access(const char *path, int mask) {
    return -fresultToErrno(f_stat(path, nullptr));
}

int serial_truncate(const char *path, off_t newsize) {
    auto fileIter = openFiles.find(std::string(path));
    if (fileIter == openFiles.end()) {
        return -ENOENT;
    }
    FRESULT rc = f_lseek(&fileIter->second, newsize);
    if (rc) {
        return -fresultToErrno(rc);
    }

    return -fresultToErrno(f_truncate(&fileIter->second));
}

int serial_ftruncate(const char *path, off_t newsize, struct fuse_file_info *) {
    return serial_truncate(path, newsize);
}

int serial_opendir(const char *path, struct fuse_file_info *fi) {
    openDirs.insert({std::string(path), DIR{}});
    FRESULT rc = f_opendir(&openDirs.at(std::string(path)), path);
    if (rc) {
        openDirs.erase(std::string(path));
    }

    return -fresultToErrno(rc);
}

int serial_release(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    // close if not already closed
    auto file = openFiles.find(std::string(path));
    if (file != openFiles.end()) {
        FRESULT rc = f_close(&file->second);
        if (rc == 0) {
            openFiles.erase(file);
        }
        return -fresultToErrno(rc);
    }
    return 0;
}

int serial_utimens(const char *path, const struct timespec tv[2]) {
    std::cerr << "utimens\n";
    return -ENOSYS;
}

// destroy will be useful if I put userData in private_data instead of a global
void serial_destroy(void *userdata) { std::cout << "destroy\n"; }

int serial_flush(const char *path, struct fuse_file_info *fi) {
    std::cout << "flush\n";
    return 0;
}

int serial_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
    (void)datasync;
    (void)fi;
    auto fileIter = openFiles.find(std::string(path));
    if (fileIter == openFiles.end()) {
        return -ENOENT;
    }
    return -fresultToErrno(f_sync(&fileIter->second));
}

int serial_releasedir(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    // close if not already closed
    auto dirIter = openDirs.find(std::string(path));
    if (dirIter != openDirs.end()) {
        FRESULT rc = f_closedir(&dirIter->second);
        if (rc == 0) {
            openDirs.erase(dirIter);
        }
        return -fresultToErrno(rc);
    }
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

int serial_lock(const char *path, struct fuse_file_info *fi, int cmd,
                struct flock *locks) {
    std::cerr << "lock\n";
    return -ENOSYS;
}

int serial_bmap(const char *path, size_t blocksize, uint64_t *blockno) {
    std::cerr << "bmap\n";
    return -ENOSYS;
}

int serial_ioctl(const char *path, int cmd, void *arg,
                 struct fuse_file_info *fi, unsigned int flags, void *data) {
    std::cerr << "ioctl\n";
    return -ENOSYS;
}

int serial_poll(const char *path, struct fuse_file_info *fi,
                struct fuse_pollhandle *ph, unsigned *reventsp) {
    std::cerr << "poll\n";
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
