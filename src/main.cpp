#define FUSE_USE_VERSION 26
extern "C" {
#include "fuse.h"
}
#include "FuseFunctions.hpp"
#include "SerialInterface.hpp"
#include "SerialWrapper.hpp" // for DEFAULT_PORT

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
    // fuse_args args = FUSE_ARGS_INIT(argc, argv);
    InitDosFsSerialConnection(DEFAULT_PORT, 115200);

    data.partitionStart = DFS_GetPtnStart(DFS_SerialReadSector, 0, data.scratch,
                                          0, NULL, NULL, &data.partitionSize);
    uint32_t rc = DFS_GetVolInfo(DFS_SerialReadSector, 0, data.scratch,
                                 data.partitionStart, &data.volInfo);
    if (rc) {
        std::cout << "failed to get volume info\n";
        return -1;
    }

    int ret = fuse_main(argc, argv, &serialOps, &data);

    return ret;
}
