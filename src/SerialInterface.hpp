#ifndef SERIALINTERFACE_H_
#define SERIALINTERFACE_H_

#include "elm-fatfs/diskio.h"
#include <stdint.h>

#ifdef __linux__
constexpr const char DEFAULT_PORT[] = "/dev/ttyUSB0";
#else
constexpr const char DEFAULT_PORT[] = "COM5";
#endif

#endif // SERIALINTERFACE_H_
