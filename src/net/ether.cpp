#include "ether.h"
#include "arp.h"
#include "ipv4.h"
#include "net_init.h"
#include "net_interface.h"
#include "sys_plat.h"

#include <cstdint>
#include <cstring>
#include <unordered_map>

namespace netstack 
{
    std::unordered_map<uint32_t, NetInterface*> kNetInterfaces;

    bool EtherPush(std::shared_ptr<PacketBuffer> pkt, PROTO_TYPE type)
    {
        EtherHdr ether_hdr;
        ether_hdr.protocol = type;
        if (type == TYPE_IPV4)
        {
            IPV4_Hdr* ipv4_hdr = pkt->GetObjectPtr<IPV4_Hdr>();
            NetInfo* src_ip_info = NetInit::GetInstance()->GetNetworkInfo(ipv4_hdr->src_ipaddr);

            bool ret = ArpPush((uint8_t*)&ipv4_hdr->src_ipaddr, (uint8_t*)&ipv4_hdr->dst_ipaddr, ether_hdr.dst_addr);
            if (ret == false)
            {
                pkt.reset();
                return false;
            }

            EtherHdr* hdr_ptr = pkt->AllocateObject<EtherHdr>();
            *hdr_ptr = ether_hdr;
            hdr_ptr->data = ipv4_hdr;
        }
        else 
        {
            Arp* arp = pkt->GetObjectPtr<Arp>();
            memcpy(ether_hdr.src_addr, arp->src_hwaddr, sizeof(ether_hdr.src_addr));
            memcpy(ether_hdr.dst_addr, arp->dst_hwaddr, sizeof(ether_hdr.dst_addr));
        }
        
        auto it = kNetInterfaces.find(*(uint32_t*)&ether_hdr.dst_addr);
        it->second->PushPacket(pkt);

        return true;
    }

    bool EtherPop(std::shared_ptr<PacketBuffer> pkt)
    {


        return true;
    }
}