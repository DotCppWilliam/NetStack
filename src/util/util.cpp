#include "util.h"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <random>

namespace netstack 
{

    void DumpHex(const unsigned char* data, size_t size)
    {
        int cnt = 0;
        int line_cnt = 1;
        for (size_t i = 0; i < size; i++, cnt++)
        {
            if (cnt == 8)
            {
                if (line_cnt != 2)
                {
                    line_cnt++;
                    printf("\t");
                }
                else if (line_cnt == 2)
                {
                    printf("\n");
                    line_cnt = 1;
                }
                cnt = 0;
            }
            
            printf("%02x ", data[i]);
            fflush(stdout);
        }
        printf("\n");
    }


    uint16_t GetRandomNum()
    {
        std::random_device device;
        std::mt19937 generator(device());
        uint16_t max_num = std::numeric_limits<uint16_t>::max();
        std::uniform_int_distribution<uint16_t> distribution(0, max_num);
        return distribution(generator);
    }
}