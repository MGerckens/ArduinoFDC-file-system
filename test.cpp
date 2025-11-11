#include "libs/SerialInterface.h"
#include "libs/SerialWrapper.h"
// extern "C"{
#include "libs/dosfs.h"
// }
#include <cassert>
#include <cstdlib>

int main(int _argc, char *_argv[]) {
    auto serial = std::make_unique<SerialWrapper>("COM5", 115200);
    const char cmd[] = "r1,1,0\r\n";
    serial->writeStr(cmd);
    bool success = false;
    bool gotReadback = false;
    bool gotNextValid = false;
    char readbackBuffer[8];
    size_t readbackBufferIdx = 0;
    do {
        char c = serial->readChar(success);

        if (success) {
            if (gotReadback && !gotNextValid) {
                gotNextValid = true;
            }
            std::cout << c;
        } else {
            if (!gotReadback || (gotReadback && gotNextValid)) {
                std::cout << " FAIL ";
                break;
            }
        }

        if (!gotReadback) {
            if (readbackBufferIdx >= sizeof(cmd)) {
                return -1;
            }
            readbackBuffer[readbackBufferIdx++] = c;
            if (!std::strncmp(cmd, readbackBuffer, 8)) {
                gotReadback = true;
            }
        } else {
        }

    } while (true);
    std::cout << "\ndone\n";
}