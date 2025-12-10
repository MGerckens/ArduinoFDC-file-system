#ifndef SERIALINTERFACE_H_
#define SERIALINTERFACE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void InitDosFsSerialConnection(const char *device, long baudRate);

/*
Read sector from image
Returns 0 OK, nonzero for any error
*/
uint32_t DFS_SerialReadSector(uint8_t *buffer, uint32_t sector, uint32_t count);

/*
Write sector to image
Returns 0 OK, nonzero for any error
*/
uint32_t DFS_SerialWriteSector(uint8_t *buffer, uint32_t sector,
                               uint32_t count);

// Override media sector functions
#define DFS_ReadSector(unit, buffer, sector, count)                            \
    DFS_SerialReadSector(buffer, sector, count)
#define DFS_WriteSector(unit, buffer, sector, count)                           \
    DFS_SerialWriteSector(buffer, sector, count)

#ifdef __cplusplus
}
#endif

#endif // SERIALINTERFACE_H_
