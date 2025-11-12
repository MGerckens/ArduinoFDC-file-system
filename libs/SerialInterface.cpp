#include "SerialInterface.h"

std::unique_ptr<SerialWrapper> serial;

static std::tuple<uint8_t, uint8_t, uint8_t> sectorToHTS(uint32_t sector) {
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

    char hexbuf[2];
    bool secondNibble = false;

    serial->writeStr(sendBuf);
    while (true) {
        char c = serial->readChar(success);
        if (success) {
            if (gotReadback && !gotNextValid) {
                gotNextValid = true;
            }else if (gotReadback){
                if(!std::isspace(c)){
                    if(recvBufferIdx >= recvBuf.size()){
                        return -1;
                    }
                    if(secondNibble){
                        hexbuf[1] = c;
                        // todo: this is silly
                        sscanf(hexbuf, "%02X", &recvBuf[recvBufferIdx++]);
                        secondNibble = false;
                    }else{
                        hexbuf[0] = c;
                        secondNibble = true;
                    }
                }
            }
            
        } else {
            if (!gotReadback) {
                std::cout << " FAIL ";
                return -1;
            }
            else if(gotNextValid){
                // std::cout << (char*)recvBuf.data() << "\n\n";
                return 0;
            }
        }

        if (!gotReadback) {
            if (readbackBufferIdx >= sendBuf.size()) {
                return -1;
            }
            readbackBuffer.push_back(c);
            // std::cout << "readback: " << readbackBuffer << "\n\n";
            if (std::equal(sendBuf.begin(), sendBuf.end(), readbackBuffer.begin())) {
                gotReadback = true;
            }
        }
    }
}

extern "C" int InitDosFsSerialConnection(const char *device, long baudRate) {
    serial = std::make_unique<SerialWrapper>(device, baudRate);
    return 0;
}

extern "C" int DFS_SerialReadSector(uint8_t *buffer, uint32_t sector, uint32_t count) {
    bool success;
    std::cout << "reading sector " << sector << "\n";
    auto [head, track, sector_] = sectorToHTS(sector);
    std::string command = std::format("r{},{},{}\r\n", track, sector_, head);

    return sendAndGetResults(command, std::span<uint8_t>(buffer, count*512));
}

extern "C" int DFS_SerialWriteSector(uint8_t *buffer, uint32_t sector, uint32_t count) {
    std::cout << "writing sector " << sector << "\n";
    auto [head, track, sector_] = sectorToHTS(sector);
    std::string command =
        std::format("w{},{},{},{}\r\n", track, sector_, head,
                    std::string_view(reinterpret_cast<char *>(buffer), count*512));
    return sendAndGetResults(command, std::span<uint8_t>(buffer, count*512));
}
