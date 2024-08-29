#pragma once

#include "net_interface.h"
#include <cstdint>
#include <unordered_map>

namespace netstack
{
    
    // 目的地, 掩码, 下一跳, 接口
    #pragma pack(1)
    struct Routing 
    {
        uint32_t dst_ip;    // 目的地
        uint32_t netmask;   // 掩码
        uint32_t next;      // 下一跳
        NetInterface netif; // 接口
    };
    #pragma pack()


    static std::unordered_map<uint32_t, Routing> kRoutingTable; // 路由表 {目的地址, 路由信息} 目的地址匹配路由表项
}