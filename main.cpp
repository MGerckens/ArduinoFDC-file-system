extern "C" {
#define FUSE_USE_VERSION 26
#include "fuse.h"
}
#include "FuseFunctions.h"

static fuse_operations serialOps = {
    .getattr = serial_getattr,
    .mknod = serial_mknod,
    .mkdir = serial_mkdir,
    .unlink = serial_unlink,
    .rmdir = serial_rmdir,
    .rename = serial_rename,
    .truncate = serial_truncate,
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
};

int main(int argc, char *argv[]) {
    fuse_args args = FUSE_ARGS_INIT(argc, argv);

    data.partitionStart = DFS_GetPtnStart(0, &data.scratchSector[0], 0, NULL,
                                          NULL, &data.partitionSize);
    uint32_t rc = DFS_GetVolInfo(0, &data.scratchSector[0], data.partitionStart,
                                 &data.volInfo);
    if (rc) {
        std::cout << "failed to get volume info\n";
        return -1;
    }

    int ret = fuse_main(argc, argv, &serialOps, &data);

    return ret;
}
