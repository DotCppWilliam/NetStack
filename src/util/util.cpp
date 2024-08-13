#include "util.h"

#include <cstdio>

namespace netstack 
{
    uint16_t Checksum16(uint32_t offset, void* buf, uint16_t len, uint32_t prev_sum, 
        int complement)
    {


        return 0;
    }


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
    }
}