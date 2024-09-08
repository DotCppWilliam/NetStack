#pragma once

#include "packet_buffer.h"
#include <memory>

namespace netstack 
{

    void UdpPush(std::shared_ptr<PacketBuffer> pkt);
    void UdpPop(std::shared_ptr<PacketBuffer> pkt);
}