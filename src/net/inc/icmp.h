#pragma once

// ICMPv4协议
#include "packet_buffer.h"
#include <memory>
namespace netstack 
{
    enum ICMP_TYPE
    {
    // 差错报文
        ICMP_NET_UNREACHABLE    =   0x00,   // 网络不可达(没有路由到目的地)
        ICMP_HOST_UNREACHABLE   =   0x01,   // 主机不可达
        ICMP_PROTO_UNREACHABLE  =   0x02,   // 协议不可达(未知协议)
        ICMP_PORT_UNREACHABLE   =   0x03,   // 端口不可达


    // 询问报文
    };

    #pragma pack(1)
    struct ICMPv4 
    {
        uint8_t     type;
        uint8_t     code;
        uint16_t    checksum;
    };
    #pragma pack()


    void IcmpPush(std::shared_ptr<PacketBuffer> pkt);
    void IcmpPop(std::shared_ptr<PacketBuffer> pkt);
}