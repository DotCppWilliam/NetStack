#pragma once

namespace net 
{
    enum NetErr_t 
    {
        NET_ERR_OK      =   0,
        NET_ERR_SYS     =   -1,
        NET_ERR_NO_MEM  =   -2,     // 没有内存
        NET_ERR_SIZE    =   -3,
    };
}