#ifndef SERIALWRAPPER_H_
#define SERIALWRAPPER_H_

#ifdef __linux__

// todo

#else
#include "libs/ceserial.h"

class SerialWrapper {
  public:
    SerialWrapper(std::string_view comPort, int baudRate)
        : underlying_(std::string(comPort), baudRate, 8, 'n', 1) {
        underlying_.Open();
    }
    char readChar(bool &success) { return underlying_.ReadChar(success); }
    bool writeStr(std::string_view str) {
        return underlying_.Write(str.data(), str.size());
    }

  private:
    ceSerial underlying_;
};

#endif // __linux__

#endif // SERIALWRAPPER_H_