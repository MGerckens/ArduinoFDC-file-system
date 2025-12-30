#ifndef HELPERS_H_
#define HELPERS_H_

#include "elm-fatfs/ff.h"
#include <cerrno>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

constexpr uint8_t unixFlagsToFatFlags(int flags) {
    uint8_t result = 0;
    if ((flags & O_ACCMODE) == O_RDONLY) {
        result |= FA_READ;
    } else if ((flags & O_ACCMODE) == O_WRONLY) {
        result |= FA_WRITE;
    } else if ((flags & O_ACCMODE) == O_RDWR) {
        result |= (FA_WRITE | FA_READ);
    }

    if (flags & O_CREAT) {
        if (flags & O_APPEND) {
            result |= FA_OPEN_APPEND;
        }
        if (flags & O_TRUNC) {
            if (flags & O_EXCL) {
                result |= FA_CREATE_NEW;
            } else {
                result |= FA_CREATE_ALWAYS;
            }
        }
    }

    return result;
}

int fresultToErrno(FRESULT result) {
    switch (result) {
    case FR_OK:
        return 0;
    case FR_DISK_ERR:
        std::cerr << "disk error\n";
        return EIO;
    case FR_INT_ERR:
        std::cerr << "internal error\n";
        return EIO; // ?
    case FR_NOT_READY:
        return EAGAIN;
    case FR_NO_FILE:
        return ENOENT;
    case FR_NO_PATH:
        return ENOENT;
    case FR_INVALID_NAME:
        std::cerr << "invalid name\n";
        return EINVAL;
    case FR_DENIED:
        return (FF_FS_READONLY) ? EROFS : EACCES; // bad
    case FR_EXIST:
        return EEXIST;
    case FR_INVALID_OBJECT:
        std::cerr << "invalid object\n";
        return EINVAL; // probably bad
    case FR_WRITE_PROTECTED:
        return EROFS;
    case FR_INVALID_DRIVE:
        return ENODEV;
    case FR_NOT_ENABLED:
        std::cerr << "not enabled\n";
        return EINVAL; // bad
    case FR_NO_FILESYSTEM:
        std::cerr << "no fs\n";
        return EINVAL; // bad
    case FR_MKFS_ABORTED:
        std::cerr << "aborted\n";
        return EINVAL;
    case FR_TIMEOUT:
        return ETIME;
    case FR_LOCKED:
        return EBUSY; // probably bad
    case FR_NOT_ENOUGH_CORE:
        return ENOMEM;
    case FR_TOO_MANY_OPEN_FILES:
        return ENFILE;
    case FR_INVALID_PARAMETER:
        std::cerr << "invalid parameter\n";
        return EINVAL;
    default:
        std::cerr << "Unknown FRESULT: " << result << "\n";
        return -1;
    }
}

time_t fatTimeToSeconds(uint16_t fatDate, uint16_t fatTime) {
    std::tm fileTime;
    // years since 1980 -> years since 1900
    fileTime.tm_year = ((fatDate & 0b1111110000000000) >> 9) + 80;
    // 1-12 -> 0-11
    fileTime.tm_mon = ((fatDate & 0b0000000111100000) >> 5) - 1;
    fileTime.tm_mday = (fatDate & 0b0000000000011111);
    fileTime.tm_hour = (fatTime & 0b1111100000000000) >> 11;
    fileTime.tm_min = (fatTime & 0b0000011111100000) >> 5;
    // two second resolution
    fileTime.tm_sec = (fatTime & 0b0000000000011111) * 2;
    fileTime.tm_isdst = -1;
    return std::mktime(&fileTime);
}

#endif
