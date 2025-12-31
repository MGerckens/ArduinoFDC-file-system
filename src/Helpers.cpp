#include "Helpers.hpp"
#include <ctime>

time_t fatTimeToUnixSeconds(WORD fatDate, WORD fatTime) {
    std::tm fileTime;
    // years since 1980 -> years since 1900
    fileTime.tm_year = ((fatDate & 0b1111111000000000) >> 9) + 80;
    // 1-12 -> 0-11
    fileTime.tm_mon = ((fatDate & 0b0000000111100000) >> 5) - 1;
    fileTime.tm_mday = (fatDate & 0b0000000000011111);
    fileTime.tm_hour = (fatTime & 0b1111100000000000) >> 11;
    fileTime.tm_min = (fatTime & 0b0000011111100000) >> 5;
    // two second resolution
    fileTime.tm_sec = (fatTime & 0b0000000000011111) * 2;
    fileTime.tm_isdst = -1;
    return std::mktime(&fileTime);
}

std::pair<WORD, WORD> unixSecondsToFatTime(time_t time) {
    std::tm *localTime = std::localtime(&time);

    WORD fatDate = 0;
    fatDate |= ((localTime->tm_year - 80) << 9) & 0b1111111000000000;
    fatDate |= ((localTime->tm_mon + 1) << 5) & 0b0000000111100000;
    fatDate |= (localTime->tm_mday) & 0b0000000000011111;

    WORD fatTime = 0;
    fatTime |= (localTime->tm_hour << 11) & 0b1111100000000000;
    fatTime |= (localTime->tm_min << 5) & 0b0000011111100000;
    fatTime |= (localTime->tm_sec / 2) & 0b0000000000011111;

    return {fatDate, fatTime};
}
