#define FUSE_USE_VERSION 26
extern "C" {
#include "fuse.h"
}
#include "FuseFunctions.hpp"

static fuse_operations serialOps = {
    .getattr = serial_getattr,
    .readlink = serial_readlink,
    .getdir = NULL,
    .mknod = serial_mknod,
    .mkdir = serial_mkdir,
    .unlink = serial_unlink,
    .rmdir = serial_rmdir,
    .symlink = serial_symlink,
    .rename = serial_rename,
    .link = serial_link,
    .chmod = serial_chmod,
    .chown = serial_chown,
    .truncate = serial_truncate,
    .utime = NULL,
    .open = serial_open,
    .read = serial_read,
    .write = serial_write,
    .statfs = serial_statfs,
    .flush = serial_flush,
    .release = serial_release,
    .fsync = serial_fsync,
    .setxattr = serial_setxattr,
    .getxattr = serial_getxattr,
    .listxattr = serial_listxattr,
    .removexattr = serial_removexattr,
    .opendir = serial_opendir,
    .readdir = serial_readdir,
    .releasedir = serial_releasedir,
    .fsyncdir = serial_fsyncdir,
    .init = serial_init,
    .destroy = serial_destroy,
    .access = serial_access,
    .create = NULL,
    .ftruncate = serial_ftruncate,
    .fgetattr = serial_fgetattr,
    .lock = serial_lock,
    .utimens = serial_utimens,
    .bmap = serial_bmap,
    .flag_nullpath_ok = 0,
    .flag_nopath = 0,
    .flag_utime_omit_ok = 0,
    .flag_reserved = 0,
    .ioctl = serial_ioctl,
    .poll = serial_poll,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &serialOps, NULL);
}
