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

std::unordered_map<std::string, FIL> openFiles;
std::unordered_map<std::string, DIR> openDirs;

// definitely needed
int serial_mkdir(const char *path, mode_t mode) {
    return -fresultToErrno(f_mkdir(path));
}

int serial_unlink(const char *path) {
    auto file = openFiles.find(std::string(path));
    if (file == openFiles.end()) {
        return -ENOENT;
    }
    FRESULT rc = f_close(&file->second);
    if (rc != 0) {
        return -fresultToErrno(rc);
    }
    openFiles.erase(file);

    return -fresultToErrno(f_unlink(path));
}

int serial_rmdir(const char *path) { return serial_unlink(path); }

int serial_read(const char *path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi) {
    auto file = openFiles.find(std::string(path));
    if (file == openFiles.end()) {
        return -ENOENT;
    }
    UINT bytesRead = 0;
    FRESULT rc = f_lseek(&file->second, offset);
    if (rc) {
        return -fresultToErrno(rc);
    }
    rc = f_read(&file->second, buf, size, &bytesRead);
    return rc ? -fresultToErrno(rc) : bytesRead;
}

int serial_write(const char *path, const char *buf, size_t size, off_t offset,
                 struct fuse_file_info *fi) {
    auto file = openFiles.find(std::string(path));
    if (file == openFiles.end()) {
        return -ENOENT;
    }
    UINT bytesWritten = 0;
    FRESULT rc = f_lseek(&file->second, offset);
    if (rc) {
        return -fresultToErrno(rc);
    }
    rc = f_write(&file->second, buf, size, &bytesWritten);
    return rc ? -fresultToErrno(rc) : bytesWritten;
}

int serial_mknod(const char *path, mode_t mode, dev_t dev) {
    (void)dev; // don't allow creating special files
    if ((mode & S_IFMT) != S_IFREG) {
        return -ENOSYS;
    }
    if(openFiles.contains(std::string(path))){
        return -EEXIST;
    }

    FIL file;
    openFiles.insert({std::string(path), FIL{}});
    FRESULT rc = f_open(&openFiles.at(std::string(path)), path, FA_CREATE_NEW);
    if (rc && rc != FR_EXIST) {
        openFiles.erase(std::string(path));
    }
    return -fresultToErrno(rc);
}

int serial_getattr(const char *path, struct stat *statbuf) {
    FILINFO info;
    FRESULT rc = f_stat(path, &info);
    if(rc){return -fresultToErrno(rc);}

    statbuf->st_dev = 0;        // assigned by fuse
    statbuf->st_ino = 0;        // assigned by fuse
    statbuf->st_mode = S_IFREG; // only regular files are supported
    statbuf->st_nlink = 0;      // FAT12 doesn't support hardlinks
    statbuf->st_uid = 0;        // or users
    statbuf->st_gid = 0;        // or groups
    statbuf->st_rdev = 0;       // not a device
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

int serial_rename(const char *path, const char *newpath) {
    // FILEINFO oldFile;
    // bool isDir = false;
    // uint32_t rc =
    //     DFS_OpenFile(serialReadSector, serialWriteSector, &data.volInfo,
    //     path,
    //                  DFS_READ, data.scratch, &oldFile);
    // if (rc == EISDIR) {
    //     isDir = true;
    // } else if (rc) {
    //     return -rc;
    // }
    // // extract old file's directory entry
    // if (serialReadSector(data.volInfo.unit, data.scratch, oldFile.dirsector,
    //                      1)) {
    //     return -EIO;
    // }
    // DIRENT oldFileDirEntry = ((PDIRENT)data.scratch)[oldFile.diroffset];

    // // get location of new file, creating it if it doesn't exist yet
    // FILEINFO newFile;
    // rc = DFS_OpenFile(serialReadSector, serialWriteSector, &data.volInfo,
    //                   newpath,
    //                   (oldFileDirEntry.attr & ATTR_DIRECTORY) ? DFS_CREATEDIR
    //                                                           :
    //                                                           DFS_CREATEFILE,
    //                   data.scratch, &newFile);

    // // can rename a directory to an existing empty directory
    // if (rc == EEXIST && isDir) {
    //     DIRINFO newDir;
    //     uint32_t rc =
    //         DFS_OpenDir(serialReadSector, &data.volInfo, newpath, &newDir);
    //     if (rc) {
    //         return -rc;
    //     }

    //     DIRENT entry{};
    //     if ((rc =
    //              DFS_GetNext(serialReadSector, &data.volInfo, &newDir,
    //              &entry)),
    //         (rc == DFS_OK && entry.name[0] != 0)) {
    //         return -EEXIST;
    //     }
    // } else if (rc) {
    //     return -rc;
    // }

    // if (serialReadSector(data.volInfo.unit, data.scratch, newFile.dirsector,
    //                      1)) {
    //     return EIO;
    // }

    // // write new name into old entry
    // std::memcpy(oldFileDirEntry.name,
    //             ((PDIRENT)data.scratch)[newFile.diroffset].name, 11);
    // // write old entry into new dir sector
    // std::memcpy(data.scratch + (newFile.diroffset * sizeof(DIRENT)),
    //             &oldFileDirEntry, sizeof(DIRENT));
    // // write new dir sector (with edited entry) back to disk
    // if (serialWriteSector(data.volInfo.unit, data.scratch, newFile.dirsector,
    //                       1)) {
    //     return -EIO;
    // }

    // // need to read old dir again bc if the old and new are in the same
    // folder,
    // // anything read previously could be outdated now
    // if (serialReadSector(data.volInfo.unit, data.scratch, oldFile.dirsector,
    //                      1)) {
    //     return -EIO;
    // }
    // // erase old entry
    // ((PDIRENT)data.scratch)[oldFile.diroffset].name[0] = 0xe5;
    // if (serialWriteSector(data.volInfo.unit, data.scratch, oldFile.dirsector,
    //                       1)) {
    //     return -EIO;
    // }
    return 0;
}

int serial_statfs(const char *path, struct statvfs *statv) {
    // (void)path;
    std::cerr << "statfs\n";
    // statv->f_type = 0x65735546; // fuse_super_magic
    // statv->f_flag |= ST_SYNCHRONOUS;
    // statv->f_bsize = 512;
    // statv->f_frsize = 512 * data.volInfo.secperclus;
    // statv->f_blocks = data.volInfo.numclusters;
    // // todo complete

    return -ENOSYS;
}

int serial_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi) {
    // DIRINFO dir{};
    // dir.scratch = data.scratch;
    // uint32_t rc = DFS_OpenDir(serialReadSector, &data.volInfo, path, &dir);
    // if (rc) {
    //     return -rc;
    // }

    // int i = 0;
    // DIRENT entry{};
    // // skip to desired offset
    // for (i = 0; i < offset; ++i) {
    //     if (DFS_GetNext(serialReadSector, &data.volInfo, &dir, &entry)) {
    //         return 0;
    //     }
    // }
    // while ((rc = DFS_GetNext(serialReadSector, &data.volInfo, &dir, &entry)),
    //        (rc == DFS_OK && entry.name[0] != 0)) {
    //     // struct stat attributes;
    //     char nullTerminatedName[12];
    //     std::memcpy(nullTerminatedName, entry.name, 11);
    //     nullTerminatedName[11] = '\0';
    //     // not totally sure if the formatting of the file name matters,
    //     // here it will be "NAME    TXT" instead of "NAME.TXT"
    //     if (filler(buf, nullTerminatedName, NULL, offset + 1)) {
    //         return 0;
    //     }
    // }

    // if (rc == DFS_EOF) {
    //     return 0;
    // } else {
    //     return -rc;
    // }
    return -ENOSYS;
}

void *serial_init(struct fuse_conn_info *conn) {
    // initSerialConnection(DEFAULT_PORT, 115200);

    // data.partitionStart = DFS_GetPtnStart(serialReadSector, 0, data.scratch,
    // 0,
    //                                       NULL, NULL, &data.partitionSize);
    // uint32_t rc = DFS_GetVolInfo(serialReadSector, 0, data.scratch,
    //                              data.partitionStart, &data.volInfo);
    f_mount(&FatFs, "", 0);

    conn->max_write = std::min<std::size_t>(UINT32_MAX, SIZE_MAX);
    conn->async_read = 0;
    conn->max_readahead = 0;
    conn->max_background = 0;
    // todo remove FUSE_CAP_ATOMIC_O_TRUNC when trunc is supported
    conn->capable =
        conn->want & ~(FUSE_CAP_ASYNC_READ | FUSE_CAP_ATOMIC_O_TRUNC);

    return nullptr;
}

int serial_open(const char *path, struct fuse_file_info *fi) {
    fi->direct_io = 1;
    FIL file;
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
    std::cerr << "opendir\n";
    return -ENOSYS;
}

int serial_release(const char *path, struct fuse_file_info *fi) {
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

// needs new functionality in dosfs
int serial_utimens(const char *path, const struct timespec tv[2]) {
    std::cerr << "utimens\n";
    return -ENOSYS;
}

// probably can leave as no-ops
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
    if(fileIter == openFiles.end()){
        return -ENOENT;
    }
    return -fresultToErrno(f_sync(&fileIter->second));
}

int serial_releasedir(const char *path, struct fuse_file_info *fi) {
    std::cout << "releasedir\n";
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
