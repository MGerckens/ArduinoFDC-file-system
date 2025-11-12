#ifndef SERIALWRAPPER_H_
#define SERIALWRAPPER_H_

#ifdef __linux__
constexpr const char DEFAULT_PORT[] = "/dev/ttyUSB0";

#include <cerrno>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>

class SerialWrapper {
  public:
    SerialWrapper(std::string_view comPort, long baudRate)
        : fd_(open(std::string(comPort).c_str(), O_RDWR)), tty_() {
        if (fd_ < 0) {
            throw std::runtime_error(
                std::format("Failed to open serial port: {} {}", errno,
                            std::strerror(errno)));
        }
        if (tcgetattr(fd_, &tty_) != 0) {
            throw std::runtime_error(std::format("Error {} from tcgetattr: {}",
                                                 errno, std::strerror(errno)));
        }
        tty_.c_cflag &= ~PARENB;
        tty_.c_cflag &= ~CSTOPB;
        tty_.c_cflag &= ~CSIZE;
        tty_.c_cflag |= CS8;
        tty_.c_cflag &= ~CRTSCTS;
        tty_.c_cflag |= CREAD | CLOCAL;
        tty_.c_lflag &= ~ICANON;
        tty_.c_lflag &= ~ECHO;
        tty_.c_lflag &= ~ECHOE;
        tty_.c_lflag &= ~ECHONL;
        tty_.c_lflag &= ~ISIG;
        tty_.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty_.c_iflag &=
            ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
        tty_.c_oflag &= ~OPOST;
        tty_.c_oflag &= ~ONLCR;
        tty_.c_cc[VTIME] = 1;
        tty_.c_cc[VMIN] = 0;
        cfsetspeed(&tty_, baudRate);
        if (tcsetattr(fd_, TCSANOW, &tty_) != 0) {
            throw std::runtime_error(std::format("Error {} from tcsetattr: {}",
                                                 errno, std::strerror(errno)));
        }
    }

    ~SerialWrapper() { close(fd_); }

    char readChar(bool &success) {
        char c = 0;
        int rc = read(fd_, &c, 1);
        success = (rc == 1);
        if (rc < 0) {
            std::cerr << "Write error: " << std::strerror(errno) << "\n";
        }

        return c;
    }
    bool writeStr(std::string_view str) {
        bool success = (write(fd_, str.data(), str.size()) == str.size());
        if (!success) {
            std::cerr << "Write error: " << std::strerror(errno) << "\n";
        }
        return success;
    }

  private:
    int fd_;
    termios tty_;
};

#else
constexpr const char DEFAULT_PORT[] = "COM5";
#include "libs/ceserial.h"

#include <cassert>
#include <string>
#include <string_view>

class SerialWrapper {
  public:
    SerialWrapper(std::string_view comPort, long baudRate)
        : underlying_(std::string(comPort), baudRate, 8, 'n', 1) {
        underlying_.Open();
    }
    char readChar(bool &success) {
        assert(underlying_.IsOpened());
        return underlying_.ReadChar(success);
    }
    bool writeStr(std::string_view str) {
        assert(underlying_.IsOpened());
        return underlying_.Write(str.data(), str.size());
    }

  private:
    ceSerial underlying_;
};

#endif // __linux__

#endif // SERIALWRAPPER_H_