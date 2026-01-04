#include "FuseFunctions.hpp"

static fuse_operations serialOps{};

int main(int argc, char *argv[]) {
    serialOps.getattr = serial_getattr;
    serialOps.readlink = serial_readlink;
    serialOps.mknod = serial_mknod;
    serialOps.mkdir = serial_mkdir;
    serialOps.unlink = serial_unlink;
    serialOps.rmdir = serial_rmdir;
    serialOps.symlink = serial_symlink;
    serialOps.rename = serial_rename;
    serialOps.link = serial_link;
    serialOps.chmod = serial_chmod;
    serialOps.chown = serial_chown;
    serialOps.truncate = serial_truncate;
    serialOps.open = serial_open;
    serialOps.read = serial_read;
    serialOps.write = serial_write;
    serialOps.statfs = serial_statfs;
    serialOps.flush = serial_flush;
    serialOps.release = serial_release;
    serialOps.fsync = serial_fsync;
    serialOps.setxattr = serial_setxattr;
    serialOps.getxattr = serial_getxattr;
    serialOps.listxattr = serial_listxattr;
    serialOps.removexattr = serial_removexattr;
    serialOps.opendir = serial_opendir;
    serialOps.readdir = serial_readdir;
    serialOps.releasedir = serial_releasedir;
    serialOps.fsyncdir = serial_fsyncdir;
    serialOps.init = serial_init;
    serialOps.destroy = serial_destroy;
    serialOps.access = serial_access;
    serialOps.ftruncate = serial_ftruncate;
    serialOps.fgetattr = serial_fgetattr;
    serialOps.utimens = serial_utimens;
    serialOps.bmap = serial_bmap;
    serialOps.ioctl = serial_ioctl;
    serialOps.poll = serial_poll;

    return fuse_main(argc, argv, &serialOps, NULL);
}
