#pragma once

namespace netstack
{
    enum PROTO_TYPE
    {
        TYPE_IPV4         = 0x0800,
        TYPE_ARP          = 0x0806,
        TYPE_CVLAN        = 0x8100,
        TYPE_SVLAN        = 0x88A8,
        TYPE_PN           = 0x8892,
        TYPE_ICMP         = 0x0001,
        TYPE_IGMP         = 0x0002,
        TYPE_TCP          = 0x0006,
        TYPE_UDP          = 0x0011,

        TYPE_NO_SUPPORT   = 0x0000,
    };
}