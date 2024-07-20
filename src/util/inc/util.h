#pragma once

#include <cstdint>

namespace util 
{
    uint16_t Checksum16(uint32_t offset, void* buf, uint16_t len, uint32_t prev_sum, 
        int complement);
}