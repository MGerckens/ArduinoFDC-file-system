#ifndef HELPERS_H_
#define HELPERS_H_
extern "C" {
#include "dosfs.h"
}
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <memory>
// #include <unordered_map>
// #include <variant>

// using OpenedEntity = std::variant<FILEINFO, DIRINFO>;
uint8_t scratchSector[SECTOR_SIZE];
struct UserData {
    uint8_t *scratch = &scratchSector[0];
    uint32_t partitionStart;
    uint32_t partitionSize;
    VOLINFO volInfo;
    // std::unordered_map<std::string, OpenedEntity> openedEntities;
};
UserData data;

constexpr int unixFlagsToDosMode(int32_t flags) {
    int returnMode = 0;
    if ((flags & O_RDONLY) || (flags & O_RDWR)) {
        returnMode |= DFS_READ;
    }
    if ((flags & O_WRONLY) || (flags & O_RDWR)) {
        returnMode |= DFS_WRITE;
    }
    if (flags & O_CREAT) {
        if (flags & O_DIRECTORY) {
            returnMode |= DFS_CREATEDIR;
        } else {
            returnMode |= DFS_CREATEFILE;
        }
    }
    return returnMode;
}

#endif
