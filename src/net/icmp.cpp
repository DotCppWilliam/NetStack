#include "icmp.h"
#include "net_err.h"

namespace netstack
{
    NetErr_t IcmpPush(uint32_t dst_ip)
    {

        return NET_ERR_OK;
    }


    NetErr_t CheckSum(ICMPv4* hdr)
    {

        return NET_ERR_OK;
    }

    void IcmpPop(std::shared_ptr<PacketBuffer> pkt)
    {
        ICMPv4* hdr = pkt->GetObjectPtr<ICMPv4>();
        if (CheckSum(hdr) != NET_ERR_OK)
        {
            pkt.reset();
            return;
        }

        
    }
}