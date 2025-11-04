#ifndef SERIALINTERFACE_H_
#define SERIALINTERFACE_H_

#include <tuple>
#include <iostream>
#include <cstdint>
#include <string>
#include <cstring>
#include <format>
#include <memory>
#include <span>
#include <algorithm>
#include <cstddef>
#include <cctype>
#include "libs/ceserial.h"

// Override media sector functions
#define DFS_ReadSector(unit,buffer,sector,count) DFS_SerialReadSector(buffer,sector,count)
#define DFS_WriteSector(unit,buffer,sector,count) DFS_SerialWriteSector(buffer,sector,count)

/*
	Attach emulation to a host-side disk image file
	Returns 0 OK, nonzero for any error
*/
extern "C" int DFS_SerialAttach(const char* device, long baudRate, long dataSize, char parityType, float nStopBits);

/*
	Read sector from image
	Returns 0 OK, nonzero for any error
*/
extern "C" int DFS_SerialReadSector(uint8_t *buffer, uint32_t sector, uint32_t count);

/*
	Write sector to image
	Returns 0 OK, nonzero for any error
*/
extern "C" int DFS_SerialWriteSector(uint8_t *buffer, uint32_t sector, uint32_t count);

#endif // SERIALINTERFACE_H_
