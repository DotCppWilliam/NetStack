#include "ether.h"
#include "net_interface.h"

#include <cstring>

namespace netstack 
{
    extern std::list<NetInterface*> kNetIfLists;

    bool EtherPush(std::shared_ptr<PacketBuffer> pkt, PROTO_TYPE type)
    {
        EtherHdr* hdr = pkt->AllocateObject<EtherHdr>();
        hdr->protocol = type;
        // TODO: 设置源mac和目标mac

        

        return true;
    }


    bool EtherPop(std::shared_ptr<PacketBuffer>& pkt)
    {


        return true;
    }
}