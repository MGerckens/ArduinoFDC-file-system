#ifndef FUSEFUNCTIONS_H_
#define FUSEFUNCTIONS_H_

#include "Helpers.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>

struct Context {
    FATFS fatFs;
    std::unordered_map<std::string, FIL> openFiles;
    std::unordered_map<std::string, DIR> openDirs;
};

extern "C" {

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include "fuse.h"

// definitely needed
int serial_mkdir(const char *path, mode_t mode);
int serial_unlink(const char *path);
int serial_rmdir(const char *path);
int serial_read(const char *path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi);
int serial_write(const char *path, const char *buf, size_t size, off_t offset,
                 struct fuse_file_info *fi);
int serial_mknod(const char *path, mode_t mode, dev_t dev);
int serial_getattr(const char *path, struct stat *statbuf);
int serial_fgetattr(const char *path, struct stat *stbuf,
                    struct fuse_file_info *fi);
int serial_rename(const char *path, const char *newpath);
int serial_statfs(const char *path, struct statvfs *statv);
int serial_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi);
void *serial_init(struct fuse_conn_info *conn);
int serial_open(const char *path, struct fuse_file_info *fi);
int serial_access(const char *path, int mask);
int serial_truncate(const char *path, off_t newsize);
int serial_ftruncate(const char *path, off_t newsize, struct fuse_file_info *);
int serial_opendir(const char *path, struct fuse_file_info *fi);
int serial_release(const char *path, struct fuse_file_info *fi);
int serial_utimens(const char *path, const struct timespec tv[2]);
void serial_destroy(void *userdata);
int serial_flush(const char *path, struct fuse_file_info *fi);
int serial_fsync(const char *path, int datasync, struct fuse_file_info *fi);
int serial_releasedir(const char *path, struct fuse_file_info *fi);
int serial_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi);

// probably impossible
int serial_setxattr(const char *path, const char *name, const char *value,
                    size_t size, int flags);
int serial_getxattr(const char *path, const char *name, char *value,
                    size_t size);
int serial_listxattr(const char *path, char *list, size_t size);
int serial_removexattr(const char *path, const char *name);
int serial_lock(const char *path, struct fuse_file_info *fi, int cmd,
                struct flock *locks);
int serial_bmap(const char *path, size_t blocksize, uint64_t *blockno);
int serial_ioctl(const char *path, int cmd, void *arg,
                 struct fuse_file_info *fi, unsigned int flags, void *data);
int serial_poll(const char *path, struct fuse_file_info *fi,
                struct fuse_pollhandle *ph, unsigned *reventsp);

// definitely impossible
int serial_symlink(const char *path, const char *link);
int serial_link(const char *path, const char *newpath);
int serial_readlink(const char *path, char *link, size_t size);
int serial_chmod(const char *path, mode_t mode);
int serial_chown(const char *path, uid_t uid, gid_t gid);
}

#endif