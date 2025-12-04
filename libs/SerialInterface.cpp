#include "SerialInterface.h"

#include <chrono>
#include <cstddef>
#include <exception>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>

static constexpr uint8_t hexToNibble(char number) {
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

static constexpr char nibbleToHex(uint8_t nibble) {
    switch (nibble) {
    case 0:
        return '0';
    case 1:
        return '1';
    case 2:
        return '2';
    case 3:
        return '3';
    case 4:
        return '4';
    case 5:
        return '5';
    case 6:
        return '6';
    case 7:
        return '7';
    case 8:
        return '8';
    case 9:
        return '9';
    case 10:
        return 'A';
    case 11:
        return 'B';
    case 12:
        return 'C';
    case 13:
        return 'D';
    case 14:
        return 'E';
    case 15:
        return 'F';
    default:
        return CHAR_MAX;
    }
}

std::unique_ptr<SerialWrapper> serial;

static std::tuple<uint8_t, uint8_t, uint8_t>
sectorToHTS(uint32_t logicalSector) {
    // todo: this is specific to 1.44MB 3.5"
    constexpr uint8_t TRACKS_PER_HEAD = 80;
    constexpr uint8_t SECTORS_PER_TRACK = 18;
    constexpr uint32_t SECTORS_PER_HEAD = 1440;

    if (logicalSector >= 2880) {
        throw std::runtime_error("invalid sector!");
    }

    uint8_t physicalSector = (logicalSector % (SECTORS_PER_TRACK)) + 1;
    uint8_t track = logicalSector / (SECTORS_PER_TRACK * 2);
    uint8_t head =
        (logicalSector % (SECTORS_PER_TRACK * 2)) >= SECTORS_PER_TRACK;

    return {head, track, physicalSector};
}

extern "C" int InitDosFsSerialConnection(const char *device, long baudRate) {
    serial = std::make_unique<SerialWrapper>(device, baudRate);
    return 0;
}

extern "C" int DFS_SerialReadSector(uint8_t *buffer, uint32_t sector,
                                    uint32_t count) {
    auto [head, track, sector_] = sectorToHTS(sector);
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
                throw std::runtime_error("Receive timed out\n");
                return -1;
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

extern "C" int DFS_SerialWriteSector(uint8_t *buffer, uint32_t sector,
                                     uint32_t count) {
    auto [head, track, sector_] = sectorToHTS(sector);

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
        throw std::runtime_error("Failed to send write command\n");
        return -1;
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
            throw std::runtime_error(
                "Timed out waiting for write confirmation\n");
            return -1;
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
