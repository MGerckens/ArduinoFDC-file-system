#ifndef FUSEWRAPPER_H_
#define FUSEWRAPPER_H_

#ifdef __linux__
extern "C" {

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include "fuse.h"
}

typedef mode_t GenericModeT;
typedef dev_t GenericDevT;
typedef uid_t GenericUidT;
typedef gid_t GenericGidT;
typedef off_t GENERIC_OFF_T;
typedef struct stat GenericStat;
typedef struct statvfs GenericStatvfs;
typedef struct timespec GenericTimespec;
#define ALL_PERMS (S_IRWXU | S_IRWXG | S_IRWXO)

#else

extern "C" {
#include "fuse/fuse.h"
#include "winfsp/winfsp.h"
}

typedef fuse_mode_t GenericModeT;
typedef fuse_dev_t GenericDevT;
typedef fuse_uid_t GenericUidT;
typedef fuse_gid_t GenericGidT;
typedef fuse_off_t GENERIC_OFF_T;
typedef struct fuse_stat GenericStat;
typedef struct fuse_statvfs GenericStatvfs;
typedef struct fuse_timespec GenericTimespec;
#define ALL_PERMS 0x777
#define USE_WINFSP_COMPAT

#endif

#endif