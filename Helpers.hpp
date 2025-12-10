#ifndef HELPERS_H_
#define HELPERS_H_
extern "C" {
#include "dosfs.h"
}
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <variant>

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
    return returnMode;
}

constexpr int dfsErrorToErrno(unsigned int err) {
    switch (err) {
    case DFS_OK:
        return 0;
    case DFS_EOF: {
        std::cerr << "EOF isn't really an error\n";
        return 0;
    }
    case DFS_WRITEPROT:
        return EACCES;
    case DFS_NOTFOUND:
        return ENOENT;
    case DFS_PATHLEN:
        return ENAMETOOLONG;
    case DFS_ALLOCNEW: {
        std::cerr << "Idk how to handle ALLOCNEW\n";
        return 0;
    }
    default: {
        std::cerr << "Unrecognized error: " << err << "\n";
        return 0;
    }
    }
}

#endif
