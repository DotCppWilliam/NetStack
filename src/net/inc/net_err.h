#pragma once

namespace netstack 
{
    enum NetErr_t 
    {
        NET_ERR_OK                      =   0,
        NET_ERR_SYS                     =   -1,
        NET_ERR_NO_MEM                  =   -2,     // 没有内存
        NET_ERR_SIZE                    =   -3,
        NET_ERR_NULL                    =   -4,
        NET_ERR_PARAM                   =   -5,
        NET_ERR_NO_OPS                  =   -6,
        NET_ERR_STATE                   =   -7,
        NET_ERR_FULL                    =   -8,
        NET_ERR_EMPTY                   =   -9,
        NET_ERR_IO                      =   -10,
        NET_ERR_BAD_ALLOC               =   -11,
        NET_ERR_INVALID_FRAME           =   -12,
        NET_ERR_DIFF_SUBNET             =   -13,    // 不是同一子网
        NET_ERR_WAIT_ARP_TIMEOUT        =   -14,    // 等待arp响应包超时
    };
}