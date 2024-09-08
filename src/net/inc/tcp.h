#pragma once

#include "packet_buffer.h"
#include <memory>

namespace netstack 
{


    void TcpPush(std::shared_ptr<PacketBuffer> pkt);
    void TcpPop(std::shared_ptr<PacketBuffer> pkt);


}