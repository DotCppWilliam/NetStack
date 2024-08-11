#pragma once

namespace net 
{
    enum NetErr_t 
    {
        NET_ERR_OK      =   0,
        NET_ERR_SYS     =   -1,
        NET_ERR_NO_MEM  =   -2,     // 没有内存
        NET_ERR_SIZE    =   -3,
        NET_ERR_NULL    =   -4,
        NET_ERR_PARAM   =   -5,
        NET_ERR_NO_OPS  =   -6,
        NET_ERR_STATE   =   -7,
        NET_ERR_FULL    =   -8,
        NET_ERR_EMPTY   =   -9,
        NET_ERR_IO      =   -10,
    };
}