#include "ether.h"
#include "arp.h"
#include "ipv4.h"
#include "net_err.h"
#include "net_interface.h"
#include "net_type.h"
#include "packet_buffer.h"

#include <cstdint>
#include <cstring>
#include <memory>

namespace netstack 
{
    extern std::map<uint32_t, NetInterface*> kNetifacesMap; // 定义在net_init.cpp中

    /**
     * @brief 检验是否是合法的以太网帧
     * 
     * @param pkt 
     * @return NetErr_t 
     */
    NetErr_t CheckEtherFrame(std::shared_ptr<PacketBuffer>& pkt)
    {
    // 1. 判断数据大小满足帧的大小,以太网帧最小64字节
        if (pkt->DataSize() < sizeof(EtherHdr))
            return NET_ERR_INVALID_FRAME;

    // 2. 校验和

        return NET_ERR_OK;
    }

    uint32_t CheckSum(std::shared_ptr<PacketBuffer>& pkt)
    {
        uint32_t checksum; 
        
        return checksum;
    }


    void EtherHost2Network(EtherHdr* ether)
    {
        ether->protocol = ntohs(ether->protocol);
    }


    void EtherNetwork2Host(EtherHdr* ether)
    {
        // TODO: 源mac和目标mac待转换
        ether->protocol = ntohl(ether->protocol);
    }




///////////////////////////////////////////////////////////////// 以下是提供给外面的接口
    /**
     * @brief 提供给上层的接口,封装成以太网帧并通过网卡发送出去
     * 
     * @param pkt 
     * @param type 
     * @param src_iface 
     * @param dst_iface_info 
     * @return NetErr_t 
     */
    NetErr_t EtherPush(std::shared_ptr<PacketBuffer> pkt, PROTO_TYPE type, NetInterface* src_iface, 
        NetInfo* dst_iface_info, NetInterface* send_iface)
    {
        EtherHdr ether_hdr;
        ether_hdr.protocol = type;
    // 设置源mac和目标mac地址
        memcpy(ether_hdr.src_addr, src_iface->GetNetInfo()->mac, sizeof(ether_hdr.src_addr));
        if (type == TYPE_IPV4)
            memcpy(ether_hdr.dst_addr, dst_iface_info->mac, sizeof(ether_hdr.dst_addr));
        else
            memset(ether_hdr.dst_addr, 0xff, sizeof(ether_hdr.dst_addr));

        ether_hdr.protocol = htons(ether_hdr.protocol);
        if (pkt->DataSize() < 46)
            pkt->FillTail(46 - pkt->DataSize());
    // 添加以太网的头和尾部校验和(根据抓包好像基本都没有校验字段)
        pkt->AddHeader(sizeof(EtherHdr), (const unsigned char*)&ether_hdr);

        return src_iface->NetTx(pkt);
    }




    /**
     * @brief 提供给下层的接口,也就是解封装数据包,如果合法则传递给上层
     * 
     * @param pkt 
     * @return true 
     * @return false 
     */
    NetErr_t EtherPop(std::shared_ptr<PacketBuffer> pkt)
    {
        if (CheckEtherFrame(pkt) != NET_ERR_OK) // 检验合法性
        {
            pkt.reset();
            return NET_ERR_INVALID_FRAME;
        }

        EtherHdr* hdr = pkt->GetObjectPtr<EtherHdr>();
        uint16_t p = hdr->protocol;
        if (p == TYPE_ARP)
        {
            printf("是Arp响应包\n");
        }
        hdr->protocol = ntohs(hdr->protocol);
        uint16_t protocol = hdr->protocol;

    // 去掉以太网的头和尾部
        pkt->RemoveHeader(sizeof(EtherHdr));

        switch (protocol)
        {
            case TYPE_IPV4: 
                IPv4Pop(pkt);   // 处理ipv4数据包
                break;
            case TYPE_ARP:
                ArpPop(pkt);    // 处理arp数据包
                break;
            default:    // 对于ipv4和arp以外的协议不处理
                pkt.reset();
                return NET_ERR_OK;
        }

        return NET_ERR_OK;
    }
}