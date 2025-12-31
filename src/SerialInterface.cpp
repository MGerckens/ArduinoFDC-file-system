#include "Helpers.hpp"
#include "SerialWrapper.hpp"
#include "elm-fatfs/diskio.h"

#include <chrono>
#include <cstddef>
#include <ctime>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>

#ifdef __linux__
constexpr const char DEFAULT_PORT[] = "/dev/ttyUSB0";
#else
constexpr const char DEFAULT_PORT[] = "COM5";
#endif

static std::unique_ptr<SerialWrapper> serial;

static std::tuple<uint8_t, uint8_t, uint8_t>
sectorToHTS(uint32_t logicalSector) {
    // todo: this is specific to 1.44MB 3.5"
    constexpr uint8_t SECTORS_PER_TRACK = 18;
    constexpr uint8_t NUM_HEADS = 2;

    if (logicalSector >= 2880) {
        return {99, 99, 99};
        std::cerr << "invalid sector!";
    }

    uint8_t physicalSector = (logicalSector % (SECTORS_PER_TRACK)) + 1;
    uint8_t track = logicalSector / (SECTORS_PER_TRACK * 2);
    uint8_t head =
        (logicalSector % (SECTORS_PER_TRACK * NUM_HEADS)) >= SECTORS_PER_TRACK;

    return {head, track, physicalSector};
}

void initSerialConnection(const char *device, long baudRate) {
    serial = std::make_unique<SerialWrapper>(device, baudRate);
}

uint32_t serialReadSector(uint8_t *buffer, uint32_t sector) {
    auto [head, track, sector_] = sectorToHTS(sector);
    if (head == 99 && track == 99 && sector_ == 99) {
        return 1;
    }
    std::string command = std::format("r{},{},{}\r\n", track, sector_, head);

    bool success = true;
    std::size_t recvBufferIdx = 0;
    uint8_t currentByte = 0;
    bool secondNibble = false;

    std::chrono::time_point start = std::chrono::system_clock::now();
    serial->writeStr(command);
    while (recvBufferIdx < 512) {
        char c = serial->readChar(success);
        if (success) {
            if (secondNibble) {
                currentByte |= hexToNibble(c);
                buffer[recvBufferIdx++] = currentByte;
                currentByte = 0;
                secondNibble = false;
            } else {
                currentByte |= (hexToNibble(c) << 4);
                secondNibble = true;
            }
        } else {
            if (std::chrono::system_clock::now() - start >
                std::chrono::seconds(5)) {
                std::cerr << "Receive timed out after receiving"
                          << recvBufferIdx << " bytes\n";
                return 1;
            }
        }
    }
    while (success) {
        // discard any extra characters after the sector is done
        // should just be \r\n
        serial->readChar(success);
    }
    return 0;
}

uint32_t serialWriteSector(const uint8_t *buffer, uint32_t sector) {
    auto [head, track, sector_] = sectorToHTS(sector);
    if (head == 99 && track == 99 && sector_ == 99) {
        return 1;
    }

    static char outHexBuffer[1024];

    std::size_t outIdx = 0;
    for (std::size_t inIdx = 0; inIdx < 512; ++inIdx) {
        outHexBuffer[outIdx++] = nibbleToHex((buffer[inIdx] & 0xF0) >> 4);
        outHexBuffer[outIdx++] = nibbleToHex(buffer[inIdx] & 0x0F);
    }

    std::string command = std::format("w{},{},{},{}\r\n", track, sector_, head,
                                      std::string_view(outHexBuffer));

    bool success = serial->writeStr(command);
    if (!success) {
        std::cerr << "Failed to send write command\n";
        return 1;
    }

    // idk why this is needed
    serial->writeStr("\r\n");

    char c;
    std::chrono::time_point start = std::chrono::system_clock::now();
    while (true) {
        c = serial->readChar(success);
        if (success && c == 'z') {
            break;
        }
        if (std::chrono::system_clock::now() - start >
            std::chrono::seconds(5)) {
            std::cerr << "Timed out waiting for write confirmation\n";
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    while (success) {
        // discard any extra characters after the sector is done
        // should just be \r\n
        serial->readChar(success);
    }
    return 0;
}
extern "C" {
DSTATUS disk_initialize(BYTE pdrv) {
    (void)pdrv;
    initSerialConnection(DEFAULT_PORT, 115200);
    return 0;
}

DSTATUS disk_status(BYTE pdrv) {
    (void)pdrv;
    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    BYTE *currentBuffPtr = buff;
    for (UINT i = 0; i < count; ++i) {
        if (serialReadSector(currentBuffPtr, sector + i)) {
            return RES_ERROR;
        }
        currentBuffPtr += 512;
    }
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    const BYTE *currentBuffPtr = buff;
    for (UINT i = 0; i < count; ++i) {
        if (serialWriteSector(currentBuffPtr, sector + i)) {
            return RES_ERROR;
        }
        currentBuffPtr += 512;
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    switch (cmd) {
    case CTRL_SYNC: {
        return RES_OK;
    }
    case GET_SECTOR_COUNT: {
        // todo: check disk geometry
        *reinterpret_cast<LBA_t *>(buff) = 2880;
        return RES_OK;
    }
    case GET_SECTOR_SIZE: {
        *reinterpret_cast<WORD *>(buff) = 512;
        return RES_OK;
    }
    case GET_BLOCK_SIZE: {
        *reinterpret_cast<DWORD *>(buff) = 1;
        return RES_OK;
    }
    default: {
        std::cerr << "Invalid command in ioctl: " << static_cast<int>(cmd)
                  << "\n";
        return RES_ERROR;
    }
    }
}

DWORD get_fattime(void) {
    auto [date, time] = unixSecondsToFatTime(std::time(nullptr));
    return (date << 16) + time;
}
}
