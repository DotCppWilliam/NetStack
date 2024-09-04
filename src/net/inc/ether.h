#pragma once
/*
        以太网帧最小为 64字节,数据区长度最小为 48字节,填充字节(值为0)到数据区末尾
        最大字节为 1518字节(4字节CRC + 14字节头部) MTU = 1500字节
    */
    /*
    字节大小：       7      1    6     6     2         0 ~ 1500               4
                | 前导 | SFD | DST | SRC | type |      data       填充字节 | FCS |  
    
    type: (IPV4: 0x0800)、(IPV6: 0x86DD)、(ARP: 0x0806)
*/

#include "net_type.h"
#include "packet_buffer.h"
#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <cstring>

namespace netstack
{
    #pragma pack(1)
//////////////////////// 以太网
// 以太网协议头
    struct EtherHdr
    {
        EtherHdr& operator=(const EtherHdr& hdr)
        {
            if (&hdr == this)
                return *this;
            memcpy(dst_addr, hdr.dst_addr, sizeof(dst_addr));
            memcpy(src_addr, hdr.src_addr, sizeof(src_addr));
            protocol = hdr.protocol;
            data = hdr.data;

            return *this;
        }
        uint8_t     dst_addr[6];    // 目标地址
        uint8_t     src_addr[6];    // 源地址
        uint16_t    protocol;       // 协议类型或长度
        void*       data = nullptr; // 数据载荷
    };

    struct EtherTail 
    {
        uint8_t checksum[4];        // 校验
    };
    

//////////////////////// VLAN 虚拟局域网
    struct Vlan
    {
        void SetCTag(int id, int prio = 0, int dei = 0)
        {
            tpid = htons(TYPE_CVLAN);
            SetTci(id, prio, dei);
        }

        void SetSTag(int id, int prio = 0, int dei = 0)
        {
            tpid = htons(TYPE_SVLAN);
            SetTci(id, prio, dei);
        }

        unsigned GetId() const 
        { return unsigned (ntohs(tci) & 0x03FF); }

        unsigned GetPrio() const 
        { return unsigned ((ntohs(tci) >> 13) & 0x0007); }

        unsigned GetDEI() const 
        { return unsigned ((ntohs(tci) >> 12) & 0x0001); }
        
        bool IsVlan() const 
        {
            uint16_t type = ntohs(tpid);
            return type = TYPE_CVLAN || type == TYPE_SVLAN;
        }

        bool IsCVlan() const 
        { return ntohs(tpid) == TYPE_CVLAN; }

        bool IsPVlan() const 
        { return ntohs(tpid) == TYPE_SVLAN; }

    private:
        void SetTci(int id, int prio, int dei)
        { tci = htons(short (id | (dei << 12) | (prio << 13))); }
    public:
        uint16_t tpid;
        uint16_t tci;
    };

//////////////////////// LLC(logical link control) 逻辑链路控制
    struct LLC
    {
        uint8_t dsap;
        uint8_t ssap;
        union 
        {
            uint16_t c16;
            uint8_t c8;
        } control;
    };

//////////////////////// SNAP(subnetwork access protocl) 子网络访问协议
    struct Oui 
    {
        uint8_t a;
        uint8_t b;
        uint8_t c;
    };

    struct Snap 
    {
        Oui oui;
        uint16_t protocol;
    };
    #pragma pack()

    /**
     * @brief 发送以太网帧
     * 
     * @param pkt 
     * @param type 
     * @return true 
     * @return false 
     */
    bool EtherPush(std::shared_ptr<PacketBuffer> pkt, PROTO_TYPE type);

    /**
     * @brief 接收以太网帧;解析是什么协议,然后交给具体的模块去处理
     * 
     * @param pkt 
     * @return true 
     * @return false 
     */
    bool EtherPop(std::shared_ptr<PacketBuffer> pkt);
}