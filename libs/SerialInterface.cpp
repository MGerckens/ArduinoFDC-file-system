#include "SerialInterface.h"

// from here: https://stackoverflow.com/a/34365963
struct Table {
    uint8_t tab[128];
    constexpr Table() : tab{} {
        tab['1'] = 1;
        tab['2'] = 2;
        tab['3'] = 3;
        tab['4'] = 4;
        tab['5'] = 5;
        tab['6'] = 6;
        tab['7'] = 7;
        tab['8'] = 8;
        tab['9'] = 9;
        tab['a'] = 10;
        tab['A'] = 10;
        tab['b'] = 11;
        tab['B'] = 11;
        tab['c'] = 12;
        tab['C'] = 12;
        tab['d'] = 13;
        tab['D'] = 13;
        tab['e'] = 14;
        tab['E'] = 14;
        tab['f'] = 15;
        tab['F'] = 15;
    }
    constexpr uint8_t operator[](char const idx) const {
        return tab[(size_t)idx];
    }
} constexpr table;

constexpr uint8_t hextoint(char number) { return table[(size_t)number]; }

constexpr char nibbleToHex(uint8_t nibble) {
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
    bool success = false;
    bool gotReadback = false;
    bool gotNextValid = false;
    std::string readbackBuffer = "";
    size_t readbackBufferIdx = 0;
    size_t recvBufferIdx = 0;

    uint8_t currentByte = 0;
    bool secondNibble = false;

    success = true;
    while (success) {
        // discard any extra characters after the sector is done
        // should just be one newline
        serial->readChar(success);
    }

    // todo remove readbacks entirely, remove this spaghetti
    serial->writeStr(sendBuf);
    if (sendBuf[0] == 'w') {
        return 0;
    }
    while (true) {
        char c = serial->readChar(success);
        if (success) {
            std::cout << c;
            if (gotReadback && !gotNextValid) {
                gotNextValid = true;
            }
            if (gotReadback) {
                if (sendBuf[0] == 'w') {
                    return 0;
                }
                if (std::isspace(c)) {
                    continue;
                }
                if (recvBufferIdx >= recvBuf.size()) {
                    std::cerr << "Received too many bytes\n";
                    return -1;
                }
                if (secondNibble) {
                    currentByte |= hextoint(c);
                    recvBuf[recvBufferIdx++] = currentByte;
                    currentByte = 0;
                    secondNibble = false;
                } else {
                    currentByte |= (hextoint(c) << 4);
                    secondNibble = true;
                }
            }

        } else {
            if (!gotReadback) {
                std::cerr << "Failed to get readback\n";
                return -1;
            } else if (gotNextValid) {
                // std::cout << (char*)recvBuf.data() << "\n\n";
                break;
            }
        }

        if (!gotReadback) {
            if (readbackBufferIdx >= sendBuf.size()) {
                std::cerr << "Readback too long\n";
                return -1;
            }
            readbackBuffer.push_back(c);
            // std::cout << "readback: " << readbackBuffer << "\n\n";
            if (std::equal(sendBuf.begin(), sendBuf.end(),
                           readbackBuffer.begin())) {
                gotReadback = true;
            }
        }
    }

    // std::cout << "discarding\n";
    success = true;
    while (success) {
        // discard any extra characters after the sector is done
        // should just be one newline
        serial->readChar(success);
    }
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
    for (size_t inIdx; inIdx < 512; ++inIdx) {
        outHexBuffer[outIdx++] = nibbleToHex(buffer[(inIdx & 0xF0) >> 4]);
        outHexBuffer[outIdx++] = nibbleToHex(buffer[inIdx & 0x0F]);
    }

    std::string command = std::format("w{},{},{},{}\r\n", track, sector_, head,
                                      std::string_view(outHexBuffer));

    bool success = serial->writeStr(command);
    if (!success) {
        std::cerr << "Failed to send write command\n";
        return -1;
    }

    std::cout << "discarding\n";
    success = true;
    while (success) {
        // discard any extra characters after the sector is done
        // should just be one newline
        serial->readChar(success);
    }
    return 0;
}
