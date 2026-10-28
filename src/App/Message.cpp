//
// Created by migue on 27/10/2026.
//

#include "Message.h"
/*
void AbstractMessage::intToCharArray(int value, char* buffer, size_t offset)
{
    for (size_t i = 0; i < sizeof(int); ++i) {
        buffer[offset + i] = (value >> (i * 8)) & 0xFF;
    }
}
int AbstractMessage::charArrayToInt(const char* buffer, size_t offset)
{
    int ret = 0;
    for (size_t i = 0; i < sizeof(int); ++i) {
        ret |= static_cast<int>(buffer[offset + i]) << (i * 8);
    }
    return ret;
}
PackageHeader AbstractMessage::getPackageHeader() const
{

    return static_cast<PackageHeader>(charArrayToInt(data, 0));
}
*/