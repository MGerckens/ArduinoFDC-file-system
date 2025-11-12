#include "SerialInterface.h"

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

std::unique_ptr<SerialWrapper> serial;

static std::tuple<uint8_t, uint8_t, uint8_t> sectorToHTS(uint32_t sector) {
    // todo: this is specific to 1.44MB 3.5"
    uint8_t head, track;
    uint32_t runningSector = sector;
    head = runningSector / 1440;
    runningSector %= 1440;
    track = runningSector / 80;
    runningSector %= 80;
    return {head, track, static_cast<uint8_t>(runningSector) + 1};
}

static int sendAndGetResults(std::string_view sendBuf,
                             std::span<uint8_t> recvBuf) {
    bool success = false;
    bool gotReadback = false;
    bool gotNextValid = false;
    std::string readbackBuffer = "";
    size_t readbackBufferIdx = 0;
    size_t recvBufferIdx = 0;

    uint8_t currentByte = 0;
    bool secondNibble = false;

    serial->writeStr(sendBuf);
    while (true) {
        char c = serial->readChar(success);
        if (success) {
            if (gotReadback && !gotNextValid) {
                gotNextValid = true;
            } else if (gotReadback) {
                if (std::isxdigit(c)) {
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
            }

        } else {
            if (!gotReadback) {
                std::cerr << "Failed to get readback\n";
                return -1;
            } else if (gotNextValid) {
                // std::cout << (char*)recvBuf.data() << "\n\n";
                return 0;
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

    return sendAndGetResults(command, std::span<uint8_t>(buffer, count * 512));
}

extern "C" int DFS_SerialWriteSector(uint8_t *buffer, uint32_t sector,
                                     uint32_t count) {
    std::cout << "writing sector " << sector << "\n";
    auto [head, track, sector_] = sectorToHTS(sector);
    std::string command = std::format(
        "w{},{},{},{}\r\n", track, sector_, head,
        std::string_view(reinterpret_cast<char *>(buffer), count * 512));
    return sendAndGetResults(command, std::span<uint8_t>(buffer, count * 512));
}
