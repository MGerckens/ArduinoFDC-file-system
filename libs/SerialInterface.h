#ifndef SERIALINTERFACE_H_
#define SERIALINTERFACE_H_

#include "libs/SerialWrapper.h"
#include <algorithm> // for equal
#include <cctype>    // for isxdigit
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <tuple>

// Override media sector functions
#define DFS_ReadSector(unit, buffer, sector, count)                            \
    DFS_SerialReadSector(buffer, sector, count)
#define DFS_WriteSector(unit, buffer, sector, count)                           \
    DFS_SerialWriteSector(buffer, sector, count)

extern "C" int InitDosFsSerialConnection(const char *device, long baudRate);

/*
        Read sector from image
        Returns 0 OK, nonzero for any error
*/
extern "C" int DFS_SerialReadSector(uint8_t *buffer, uint32_t sector,
                                    uint32_t count);

/*
        Write sector to image
        Returns 0 OK, nonzero for any error
*/
extern "C" int DFS_SerialWriteSector(uint8_t *buffer, uint32_t sector,
                                     uint32_t count);

#endif // SERIALINTERFACE_H_
