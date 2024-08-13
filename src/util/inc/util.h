#pragma once

#include <cstdint>
#include <cstddef>

namespace netstack 
{
    uint16_t Checksum16(uint32_t offset, void* buf, uint16_t len, uint32_t prev_sum, 
        int complement);

    void DumpHex(const unsigned char* data, size_t size);
}