#pragma once

#include <cstddef>
#include <ios>
#include <sstream>

namespace netstack 
{

    void DumpHex(const unsigned char* data, size_t size);

    uint16_t GetRandomNum();

    void MacEndianConvert(uint8_t* mac);

    template <typename T>
    std::string Num2HexStr(T num)
    {
        size_t byte_size = sizeof(T);
        char* num_ptr = (char*)&num;
        std::stringstream ssm;
        ssm << std::uppercase << std::hex << num;
        return std::string("0x") + ssm.str();
    }
}