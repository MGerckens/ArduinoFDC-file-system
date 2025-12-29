#ifndef SERIALINTERFACE_H_
#define SERIALINTERFACE_H_

#include <stdint.h>
#include "elm-fatfs/diskio.h"

#ifdef __linux__
constexpr const char DEFAULT_PORT[] = "/dev/ttyUSB0";
#else
constexpr const char DEFAULT_PORT[] = "COM5";
#endif

// void initSerialConnection(const char *device, long baudRate);

// /*
// Read sector from image
// Returns 0 OK, nonzero for any error
// */
// uint32_t serialReadSector(uint8_t _, uint8_t *buffer, uint32_t sector,
//                           uint32_t count);

// /*
// Write sector to image
// Returns 0 OK, nonzero for any error
// */
// uint32_t serialWriteSector(uint8_t _, const uint8_t *buffer, uint32_t sector,
//                            uint32_t count);

#endif // SERIALINTERFACE_H_
