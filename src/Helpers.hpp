#ifndef HELPERS_H_
#define HELPERS_H_
// extern "C" {
// #include "dosfs.h"
// }
#include "elm-fatfs/ff.h"
#include <cerrno>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

// using OpenedEntity = std::variant<FIL, DIR>;
// uint8_t scratchSector[512];
// struct UserData {
//     uint8_t *scratch = &scratchSector[0];
//     uint32_t partitionStart;
//     uint32_t partitionSize;
//     // VOLINFO volInfo;
//     // std::unordered_map<std::string, OpenedEntity> openedEntities;
// };
// UserData data;

FATFS FatFs;

constexpr uint8_t unixFlagsToFatFlags(int flags) {
    switch (flags) {
    case O_RDONLY:
        // r
        return FA_READ;
    case O_WRONLY:
        // w
        return FA_WRITE;
    case O_RDWR:
        // r+
        return FA_READ | FA_WRITE;
    case (O_RDWR | O_CREAT | O_TRUNC):
        // w+
        return FA_CREATE_ALWAYS | FA_WRITE | FA_READ;
    case (O_WRONLY | O_CREAT | O_APPEND):
        // a
        return FA_OPEN_APPEND | FA_WRITE;
    case (O_RDWR | O_CREAT | O_APPEND):
        // a+
        return FA_OPEN_APPEND | FA_WRITE | FA_READ;
    case (O_WRONLY | O_CREAT | O_TRUNC | O_EXCL):
        // wx
        return FA_CREATE_NEW | FA_WRITE | FA_READ;
    case (O_RDWR | O_CREAT | O_TRUNC | O_EXCL):
        // w+x
        return FA_CREATE_NEW | FA_WRITE | FA_READ;
    case (O_WRONLY | O_CREAT | O_APPEND | O_EXCL):
        // ax
        return FA_OPEN_APPEND | FA_WRITE;
    case (O_RDWR | O_CREAT | O_APPEND | O_EXCL):
        // a+x
        return FA_OPEN_APPEND | FA_WRITE | FA_READ;
    default:
        std::cerr << "got invalid mode " << flags << "\n";
        return 0;
    }
}

int fresultToErrno(FRESULT result) {
    switch (result) {
    case FR_OK:
        return 0;
    case FR_DISK_ERR:
        return EIO;
    case FR_INT_ERR:
        return EIO; // ?
    case FR_NOT_READY:
        return EAGAIN;
    case FR_NO_FILE:
        return ENOENT;
    case FR_NO_PATH:
        return ENOENT;
    case FR_INVALID_NAME:
        return EINVAL;
    case FR_DENIED:
        return (FF_FS_READONLY) ? EROFS : EACCES; // bad
    case FR_EXIST:
        return EEXIST;
    case FR_INVALID_OBJECT:
        return EINVAL; // probably bad
    case FR_WRITE_PROTECTED:
        return EROFS;
    case FR_INVALID_DRIVE:
        return ENODEV;
    case FR_NOT_ENABLED:
        return EINVAL; // bad
    case FR_NO_FILESYSTEM:
        return EINVAL; // bad
    case FR_MKFS_ABORTED:
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
