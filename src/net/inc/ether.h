#pragma once

#include <cstdint>
#include <string>

namespace netstack
{
    /*
        以太网帧最小为 64字节,数据区长度最小为 48字节,填充字节(值为0)到数据区末尾
        最大字节为 1518字节(4字节CRC + 14字节头部) MTU = 1500字节
    */
    /*
    字节大小：       7      1    6     6     2         0 ~ 1500               4
                | 前导 | SFD | DST | SRC | type |      data       填充字节 | FCS |  
    
    type: (IPV4: 0x0800)、(IPV6: 0x86DD)、(ARP: 0x0806)
    */

    // 以太网协议头
    struct __attribute__((packed)) EtherHdr
    {
        uint8_t     dst_addr[6];    // 目标地址
        uint8_t     src_addr[6];    // 源地址
        uint16_t    protocol;       // 协议类型或长度
    };

    
    struct __attribute__((packed)) EtherPacket
    {
        EtherHdr hdr;   // 14
        std::string data;       // 数据负载
        uint8_t checksum[4];   // 校验
    };
}