#include "FuseFunctions.hpp"
#include "elm-fatfs/ff.h"
#include <cerrno>
#include <cmath>   // for ceil
#include <cstring> // for memcpy
#include <iostream>
#include <tuple> // for tie

#define CONTEXT (reinterpret_cast<Context *>(fuse_get_context()->private_data))

extern "C" {

int serial_mkdir(const char *path, GenericModeT mode) {
    return -fresultToErrno(f_mkdir(path));
}

int serial_unlink(const char *path) {
    auto fileIter = CONTEXT->openFiles.find(std::string(path));
    if (fileIter != CONTEXT->openFiles.end()) {
        FRESULT rc = f_close(&fileIter->second);
        if (rc != 0) {
            return -fresultToErrno(rc);
        }
        CONTEXT->openFiles.erase(fileIter);
    }

    return -fresultToErrno(f_unlink(path));
}

int serial_rmdir(const char *path) {
    auto dirIter = CONTEXT->openDirs.find(std::string(path));
    if (dirIter != CONTEXT->openDirs.end()) {
        FRESULT rc = f_closedir(&dirIter->second);
        if (rc != 0) {
            return -fresultToErrno(rc);
        }
        CONTEXT->openDirs.erase(dirIter);
    }

    return -fresultToErrno(f_unlink(path));
}

int serial_read(const char *path, char *buf, size_t size, GenericOffT offset,
                struct fuse_file_info *fi) {
    auto fileIter = CONTEXT->openFiles.find(std::string(path));
    if (fileIter == CONTEXT->openFiles.end()) {
        return -ENOENT;
    }
    UINT bytesRead = 0;
    FRESULT rc = f_lseek(&fileIter->second, offset);
    if (rc) {
        return -fresultToErrno(rc);
    }
    rc = f_read(&fileIter->second, buf, size, &bytesRead);
    f_rewind(&fileIter->second);
    return rc ? -fresultToErrno(rc) : bytesRead;
}

int serial_write(const char *path, const char *buf, size_t size,
                 GenericOffT offset, struct fuse_file_info *fi) {
    auto fileIter = CONTEXT->openFiles.find(std::string(path));
    if (fileIter == CONTEXT->openFiles.end()) {
        return -ENOENT;
    }
    UINT bytesWritten = 0;
    FRESULT rc = f_lseek(&fileIter->second, offset);
    if (rc) {
        return -fresultToErrno(rc);
    }
    rc = f_write(&fileIter->second, buf, size, &bytesWritten);
    f_rewind(&fileIter->second);
    return rc ? -fresultToErrno(rc) : bytesWritten;
}

int serial_mknod(const char *path, GenericModeT mode, GenericDevT dev) {
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

int serial_getattr(const char *path, GenericStat *statbuf) {
    FILINFO info;
    FRESULT rc = f_stat(path, &info);
    if (rc) {
        return -fresultToErrno(rc);
    }

    statbuf->st_dev = 0; // assigned by fuse
    statbuf->st_ino = 0; // assigned by fuse
    // give everyone all permissions
    statbuf->st_mode = ALL_PERMS;
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

    GenericTimespec tempTime;
    tempTime.tv_sec = fatTimeToUnixSeconds(info.fdate, info.ftime);
    tempTime.tv_nsec = 0;
    statbuf->st_mtim = tempTime;
    // FAT doesn't support access time
    statbuf->st_atim = statbuf->st_mtim;
    // use create time for ctime
    tempTime.tv_sec = fatTimeToUnixSeconds(info.crdate, info.crtime);
    statbuf->st_ctim = tempTime;

    return 0;
}

int serial_fgetattr(const char *path, GenericStat *stbuf,
                    struct fuse_file_info *fi) {
    (void)fi;
    return serial_getattr(path, stbuf);
}

int serial_rename(const char *path, const char *newpath) {
    if (CONTEXT->openFiles.contains(std::string(path))) {
        std::cerr << "cannot rename an open file\n";
        return -EINVAL;
    }
    return -fresultToErrno(f_rename(path, newpath));
}

int serial_statfs(const char *path, GenericStatvfs *statv) {
    (void)path;
    statv->f_bsize = 512 * CONTEXT->fatFs.csize;
    statv->f_frsize = statv->f_bsize;
    statv->f_blocks = CONTEXT->fatFs.n_fatent - 2;
    statv->f_bfree = CONTEXT->fatFs.free_clst;
    statv->f_bavail = statv->f_ffree;

    statv->f_files = 0; // FAT doesn't use inodes
    statv->f_ffree = 0;
    statv->f_favail = 0;

#ifndef USE_WINFSP_COMPAT
    statv->f_flag = (ST_SYNCHRONOUS | ST_NODEV);
    if (FF_FS_READONLY) {
        statv->f_flag |= ST_RDONLY;
    }
    statv->f_type = 0x65735546; // fuse_super_magic
#endif
    statv->f_namemax = 99;
    return 0;
}

int serial_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   GenericOffT offset, struct fuse_file_info *fi) {
    auto dirIter = CONTEXT->openDirs.find(std::string(path));
    if (dirIter == CONTEXT->openDirs.end()) {
        return -ENOENT;
    }

    FILINFO info;
    FRESULT rc;
    int i = 0;
    for (i = 0; i < offset; ++i) {
        rc = f_readdir(&dirIter->second, &info);
        if (rc) {
            f_rewinddir(&dirIter->second);
            return -fresultToErrno(rc);
        }
        if (info.fname[0] == '\x00') {
            f_rewinddir(&dirIter->second);
            return 0;
        }
    }
    while ((rc = f_readdir(&dirIter->second, &info)), (rc == FR_OK)) {
        if (rc) {
            return -fresultToErrno(rc);
        }
        if (info.fname[0] == '\x00') {
            f_rewinddir(&dirIter->second);
            return 0;
        }
        if (filler(buf, info.fname, NULL, ++i)) {
            f_rewinddir(&dirIter->second);
            return 0;
        }
    }
    f_rewinddir(&dirIter->second);
    return 0;
}

void *serial_init(struct fuse_conn_info *conn) {
    Context *ctx = new Context;
    f_mount(&ctx->fatFs, "", 0);

    conn->max_write = 0;
    conn->async_read = 0;
    conn->max_readahead = 0;
#ifndef USE_WINFSP_COMPAT
    conn->max_background = 0;
#endif
    conn->want |= FUSE_CAP_BIG_WRITES;

    return ctx;
}

int serial_open(const char *path, struct fuse_file_info *fi) {
    fi->direct_io = 1;
    // for ??reasons??, opening a FIL and then adding it to the map causes it to
    // be corrupted inserting an empty FIL and then opening it works
    CONTEXT->openFiles.insert({std::string(path), FIL{}});
    FRESULT rc = f_open(&(CONTEXT->openFiles.at(std::string(path))), path,
                        unixFlagsToFatFlags(fi->flags));
    if (rc) {
        CONTEXT->openFiles.erase(std::string(path));
    }

    return -fresultToErrno(rc);
}

int serial_access(const char *path, int mask) {
    return -fresultToErrno(f_stat(path, nullptr));
}

int serial_truncate(const char *path, GenericOffT newsize) {
    auto fileIter = CONTEXT->openFiles.find(std::string(path));
    if (fileIter == CONTEXT->openFiles.end()) {
        return -ENOENT;
    }
    FRESULT rc = f_lseek(&fileIter->second, newsize);
    if (rc) {
        return -fresultToErrno(rc);
    }
    rc = f_truncate(&fileIter->second);
    if (rc) {
        return -fresultToErrno(rc);
    }
    return -fresultToErrno(f_rewind(&fileIter->second));
}

int serial_ftruncate(const char *path, GenericOffT newsize,
                     struct fuse_file_info *) {
    return serial_truncate(path, newsize);
}

int serial_opendir(const char *path, struct fuse_file_info *fi) {
    CONTEXT->openDirs.insert({std::string(path), DIR{}});
    FRESULT rc = f_opendir(&(CONTEXT->openDirs.at(std::string(path))), path);
    if (rc) {
        CONTEXT->openDirs.erase(std::string(path));
    }

    return -fresultToErrno(rc);
}

int serial_release(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    // close if not already closed
    auto file = CONTEXT->openFiles.find(std::string(path));
    if (file != CONTEXT->openFiles.end()) {
        FRESULT rc = f_close(&file->second);
        if (rc == 0) {
            CONTEXT->openFiles.erase(file);
        }
        return -fresultToErrno(rc);
    }
    return 0;
}

int serial_utimens(const char *path, const GenericTimespec tv[2]) {
    FILINFO times;
    times.crdate = 0;
    times.crtime = 0;
    // FAT doesn't support access time
    std::tie(times.fdate, times.ftime) = unixSecondsToFatTime(tv[1].tv_sec);
    return -fresultToErrno(f_utime(path, &times));
}

void serial_destroy(void *userdata) {
    delete reinterpret_cast<Context *>(userdata);
}

int serial_flush(const char *path, struct fuse_file_info *fi) {
    std::cout << "flush\n";
    return 0;
}

int serial_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
    (void)datasync;
    (void)fi;
    auto fileIter = CONTEXT->openFiles.find(std::string(path));
    if (fileIter == CONTEXT->openFiles.end()) {
        return -ENOENT;
    }
    return -fresultToErrno(f_sync(&fileIter->second));
}

int serial_releasedir(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    // close if not already closed
    auto dirIter = CONTEXT->openDirs.find(std::string(path));
    if (dirIter != CONTEXT->openDirs.end()) {
        FRESULT rc = f_closedir(&dirIter->second);
        if (rc == 0) {
            CONTEXT->openDirs.erase(dirIter);
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
int serial_chmod(const char *path, GenericModeT mode) {
    std::cerr << "chmod\n";
    return -ENOSYS;
}
int serial_chown(const char *path, GenericUidT uid, GenericGidT gid) {
    std::cerr << "chown\n";
    return -ENOSYS;
}
}
