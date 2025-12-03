#include "SerialInterface.h"

#include <chrono>
#include <thread>

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

static int sendReadAndGetResults(std::string_view sendBuf,
                                 std::span<uint8_t> recvBuf) {
    bool success = true;
    size_t recvBufferIdx = 0;
    uint8_t currentByte = 0;
    bool secondNibble = false;

    if (recvBuf.size() < 512) {
        std::cerr << "Receive buffer too small!";
        return -1;
    }

    std::chrono::time_point start = std::chrono::system_clock::now();
    serial->writeStr(sendBuf);
    while (recvBufferIdx < 512) {
        char c = serial->readChar(success);
        if (success) {
            std::cout << c;
            if (secondNibble) {
                currentByte |= hexToNibble(c);
                recvBuf[recvBufferIdx++] = currentByte;
                currentByte = 0;
                secondNibble = false;
            } else {
                currentByte |= (hexToNibble(c) << 4);
                secondNibble = true;
            }
        } else {
            if (std::chrono::system_clock::now() - start >
                std::chrono::seconds(2)) {
                std::cerr << "Receive timed out\n";
                return -1;
            } else {
                std::cerr << "Failed to read character\n";
                return -1;
            }
        }
    }
    while (success) {
        // discard any extra characters after the sector is done
        // should just be \r\n
        serial->readChar(success);
    }
    std::cout << "\n";
    return 0;
}

extern "C" int InitDosFsSerialConnection(const char *device, long baudRate) {
    serial = std::make_unique<SerialWrapper>(device, baudRate);
    return 0;
}

extern "C" int DFS_SerialReadSector(uint8_t *buffer, uint32_t sector,
                                    uint32_t count) {
    bool success;
    std::cout << "reading sector " << sector << "\n";
    auto [head, track, sector_] = sectorToHTS(sector);
    std::string command = std::format("r{},{},{}\r\n", track, sector_, head);

    return sendReadAndGetResults(command,
                                 std::span<uint8_t>(buffer, count * 512));
}

extern "C" int DFS_SerialWriteSector(uint8_t *buffer, uint32_t sector,
                                     uint32_t count) {
    std::cout << "writing sector " << sector << "\n";
    auto [head, track, sector_] = sectorToHTS(sector);

    static char outHexBuffer[1024];

    size_t outIdx = 0;
    for (size_t inIdx = 0; inIdx < 512; ++inIdx) {
        outHexBuffer[outIdx++] = nibbleToHex((buffer[inIdx] & 0xF0) >> 4);
        outHexBuffer[outIdx++] = nibbleToHex(buffer[inIdx] & 0x0F);
    }

    std::string command = std::format("w{},{},{},{}\r\n", track, sector_, head,
                                      std::string_view(outHexBuffer));
    std::cout << "writing:\n" << command << "\n";

    bool success = serial->writeStr(command);
    if (!success) {
        std::cerr << "Failed to send write command\n";
        return -1;
    }

    // idk why this is needed
    serial->writeStr("\r\n");

    char c;
    std::chrono::time_point start = std::chrono::system_clock::now();
    while (true) {
        c = serial->readChar(success);
        if (success && c == 'z') {
            return 0;
        }
        if (std::chrono::system_clock::now() - start >
            std::chrono::seconds(5)) {
            std::cerr << "Timed out waiting for write confirmation\n";
            return -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return -1;
}
