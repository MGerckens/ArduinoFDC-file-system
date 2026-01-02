#ifndef FUSEFUNCTIONS_H_
#define FUSEFUNCTIONS_H_

#include "FuseWrapper.hpp"
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

// definitely needed
int serial_mkdir(const char *path, GenericModeT mode);
int serial_unlink(const char *path);
int serial_rmdir(const char *path);
int serial_read(const char *path, char *buf, size_t size, GENERIC_OFF_T offset,
                struct fuse_file_info *fi);
int serial_write(const char *path, const char *buf, size_t size,
                 GENERIC_OFF_T offset, struct fuse_file_info *fi);
int serial_mknod(const char *path, GenericModeT mode, dev_t dev);
int serial_getattr(const char *path, GenericStat *statbuf);
int serial_fgetattr(const char *path, GenericStat *stbuf,
                    struct fuse_file_info *fi);
int serial_rename(const char *path, const char *newpath);
int serial_statfs(const char *path, GenericStatvfs *statv);
int serial_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   GENERIC_OFF_T offset, struct fuse_file_info *fi);
void *serial_init(struct fuse_conn_info *conn);
int serial_open(const char *path, struct fuse_file_info *fi);
int serial_access(const char *path, int mask);
int serial_truncate(const char *path, GENERIC_OFF_T newsize);
int serial_ftruncate(const char *path, GENERIC_OFF_T newsize,
                     struct fuse_file_info *);
int serial_opendir(const char *path, struct fuse_file_info *fi);
int serial_release(const char *path, struct fuse_file_info *fi);
int serial_utimens(const char *path, const GenericTimespec tv[2]);
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
;
int serial_bmap(const char *path, size_t blocksize, uint64_t *blockno);
int serial_ioctl(const char *path, int cmd, void *arg,
                 struct fuse_file_info *fi, unsigned int flags, void *data);
int serial_poll(const char *path, struct fuse_file_info *fi,
                struct fuse_pollhandle *ph, unsigned *reventsp);

// definitely impossible
int serial_symlink(const char *path, const char *link);
int serial_link(const char *path, const char *newpath);
int serial_readlink(const char *path, char *link, size_t size);
int serial_chmod(const char *path, GenericModeT mode);
int serial_chown(const char *path, GenericUidT uid, GenericGidT gid);
}

#endif