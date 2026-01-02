#ifndef FUSEWRAPPER_H_
#define FUSEWRAPPER_H_

#ifdef __linux__
extern "C" {

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include "fuse.h"
}

#define GENERIC_MODE_T mode_t
#define GENERIC_DEV_T dev_t
#define GENERIC_UID_T uid_t
#define GENERIC_GID_T gid_t
#define GENERIC_OFF_T off_t
#define GENERIC_STAT stat
#define GENERIC_STATVFS statvfs
#define GENERIC_TIMESPEC timespec
#define ALL_PERMS (S_IRWXU | S_IRWXG | S_IRWXO)

#else

extern "C" {
#include "fuse/fuse.h"
#include "winfsp/winfsp.h"
}

#define GENERIC_MODE_T fuse_mode_t
#define GENERIC_DEV_T fuse_dev_t
#define GENERIC_UID_T fuse_uid_t
#define GENERIC_GID_T fuse_gid_t
#define GENERIC_OFF_T fuse_off_t
#define GENERIC_STAT fuse_stat
#define GENERIC_STATVFS fuse_statvfs
#define GENERIC_TIMESPEC fuse_timespec
#define USE_WINFSP_COMPAT

#define ALL_PERMS 0x777

#endif

#endif