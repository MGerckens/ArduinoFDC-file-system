#ifndef HELPERS_H_
#define HELPERS_H_

#include "elm-fatfs/ff.h"
#include <cerrno>
#include <climits>
#include <cstdint>
#include <fcntl.h> // for open modes
#include <iostream>
#include <utility>

constexpr uint8_t hexToNibble(char number) {
    switch (number) {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case 'A':
        return 10;
    case 'B':
        return 11;
    case 'C':
        return 12;
    case 'D':
        return 13;
    case 'E':
        return 14;
    case 'F':
        return 15;
    default:
        return CHAR_MAX;
    }
}

constexpr char table[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
constexpr char nibbleToHex(uint8_t nibble) {
    if (nibble > 15) {
        return CHAR_MAX;
    }
    return table[nibble];
}

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

constexpr int fresultToErrno(FRESULT result) {
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

time_t fatTimeToUnixSeconds(WORD fatDate, WORD fatTime);
std::pair<WORD, WORD> unixSecondsToFatTime(time_t time);

#endif
