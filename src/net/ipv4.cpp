#include "ipv4.h"
#include "ether.h"
#include "icmp.h"
#include "net_err.h"
#include "net_interface.h"
#include "net_type.h"
#include "packet_buffer.h"
#include "routing.h"
#include "tcp.h"
#include "udp.h"
#include "util.h"

#include <arpa/inet.h>
#include <atomic>
#include <memory>


#define IPV4_DATA_MAX_SIZE  (1480)      // ipv4数据载荷最大支持1480字节: MTU(1500) - ipv4头部(20字节)

namespace netstack 
{
    extern std::map<uint32_t, NetInterface*> kNetifacesMap;    // 定义在net_init.cpp中

    static uint16_t kDataIdentification = 0;    // 数据包的标识
    static uint16_t kDefaultTTL = 64;           // 默认TTL为64(linux默认为这个值)

    // key: 标识,   value: 收到的报文
    static std::map<uint16_t, MsgReassembly> kMsgReassemblyMap; // 接收重组分片的map
    static std::atomic<pthread_t> kMsgRessemblyLock = 0;            // 用于对上面的锁




/////////////////////////////////////////////////////////////// 函数实现
    /**
     * @brief 计算ipv4的校验和
     * 
     * @param hdr 
     * @return uint16_t 
     */
    uint16_t CheckSum(IPV4_Hdr* hdr)
    {
        uint32_t sum = 0;
        uint8_t* hdr_ptr = reinterpret_cast<uint8_t*>(hdr);
        for (int i = 0; i < 20; i += 2)
        {
            uint16_t word = (hdr_ptr[i] << 8) + hdr_ptr[i + 1];
            sum += word;
        }
        while (sum >> 16)
            // 低16位与溢出部分重新累加
            sum = (sum & 0xffff) + (sum >> 16);

        // 将结果进行反码
        return static_cast<uint16_t>(~sum);
    }


    NetErr_t CheckIpv4(std::shared_ptr<PacketBuffer>& pkt)
    {

        return NET_ERR_OK;
    }

    // 将ipv4头转成网络字节序
    void Ipv4Host2Network(IPV4_Hdr* hdr)
    {
        hdr->total_length = htons(hdr->total_length);
        hdr->identification = htons(hdr->identification);
        hdr->flags_fragment = htons(hdr->flags_fragment);
        hdr->head_checksum = htons(hdr->head_checksum);
    }

    /**
     * @brief 将ipv4头部转换成网络字节序
     * 
     * @param hdr 
     */
    void Ipv4Network2Host(IPV4_Hdr* hdr)
    {
        hdr->total_length = ntohs(hdr->total_length);
        hdr->identification = ntohs(hdr->identification);
        hdr->flags_fragment = ntohs(hdr->flags_fragment);
        hdr->head_checksum = ntohs(hdr->head_checksum);
    }

    /**
     * @brief 数据报分片
     * 
     * @param pkt 
     */
    void Ipv4Fragment(std::shared_ptr<PacketBuffer> pkt, IPV4_Hdr hdr, NetInterface* send_iface)
    {
        size_t data_size = pkt->TotalSize();
        size_t chunk_cnt = data_size / IPV4_DATA_MAX_SIZE;
        chunk_cnt += (data_size % IPV4_DATA_MAX_SIZE) != 0 ? 1 : 0;
        size_t index = 0;

        size_t chunk_size;
        for (size_t i = 0; i < chunk_cnt; i++)
        {
            chunk_size = data_size < IPV4_DATA_MAX_SIZE ? data_size : IPV4_DATA_MAX_SIZE;
            data_size -= chunk_size;
            std::shared_ptr<PacketBuffer> chunk_pkt =
                std::make_shared<PacketBuffer>(chunk_size);
            
            pkt->Read(chunk_pkt->GetObjectPtr(), chunk_size);
            pkt->Seek(chunk_size);

            hdr.flags = FRAG_MORE_FRAGMENT;     // 允许分片,还有更多分片
            hdr.fragment_offset = (i * IPV4_DATA_MAX_SIZE);
            hdr.total_length = chunk_size + 20; // 总长度=数据长度+头部大小
            hdr.head_checksum = CheckSum(&hdr);
            
            Ipv4Host2Network(&hdr);
            pkt->AddHeader(sizeof(IPV4_Hdr), (const unsigned char*)&hdr);

            EtherPush(pkt, TYPE_IPV4, kNetifacesMap[hdr.src_ipaddr], send_iface->GetNetInfo(), send_iface);
        }
        pkt.reset();
    }


///////////////////////////////////////////////////////// 提供给外部的接口

    /**
     * @brief 提供给上层传输层使用,比如UDP、TCP、ICMP.
     *        给上层数据增加ipv4头,如果数据包比较大则进行分片
     * 
     * @param pkt 上层的数据包
     * @param src_ip 源ip地址,可以为空表示任意源地址
     * @param dst_ip 目标地址
     * @param type 上层使用的是什么协议
     */
    NetErr_t IPv4Push(std::shared_ptr<PacketBuffer> pkt, uint32_t src_ip, uint32_t dst_ip, PROTO_TYPE type)
    {
        Routing routing = GetRouting(dst_ip);
        IPV4_Hdr hdr;
        hdr.version = 4;
        hdr.header_length = 5;
        hdr.ds = DSCP_CS0;
        hdr.ecn = 0;
        hdr.identification = GetRandomNum();
        hdr.ttl = kDefaultTTL;
        hdr.protocol = type;
        hdr.src_ipaddr = src_ip;
        hdr.dst_ipaddr = dst_ip;
        hdr.head_checksum = 0;

        if (pkt->DataSize() > IPV4_DATA_MAX_SIZE)   // 分片
        {
            Ipv4Fragment(pkt, hdr, routing.iface_);
            return NET_ERR_OK;
        }

        Ipv4Host2Network(&hdr);
        pkt->AddHeader(sizeof(hdr), (const unsigned char*)&hdr);
        hdr.head_checksum = CheckSum(&hdr);
        EtherPush(pkt, TYPE_IPV4, kNetifacesMap[src_ip], routing.iface_->GetNetInfo(), routing.iface_);
        
        return NET_ERR_OK;
    }


    NetErr_t IPv4Pop(std::shared_ptr<PacketBuffer> pkt)
    {
        return NET_ERR_OK;

        NetErr_t ret = CheckIpv4(pkt);
        if (ret != NET_ERR_OK)
            return ret;

        IPV4_Hdr* hdr = pkt->GetObjectPtr<IPV4_Hdr>();
        Ipv4Network2Host(hdr);
        MsgReassembly complete_msg;
        
        // 是分片数据包
        if (hdr->flags == FRAG_MORE_FRAGMENT || 
            hdr->flags_fragment == FRAG_NO_MORE_FRAGMENT)
        {
            std::map<uint16_t, MsgReassembly>::iterator it;
            MsgReassembly last_fragment;
            uint16_t fragment_off = hdr->fragment_offset * 8;
            pthread_t old = 0;
            pthread_t self = pthread_self();
            
            if (hdr->flags == FRAG_NO_MORE_FRAGMENT)    // 最后一片设置总大小
                last_fragment.total_size = hdr->fragment_offset * 8 + hdr->total_length;

            // 获取锁
            while (!kMsgRessemblyLock.compare_exchange_strong(old, self));
            it = kMsgReassemblyMap.find(hdr->identification);
            
            // 表示当前是分片的第一片
            if (it == kMsgReassemblyMap.end())  
            {
                last_fragment.recv_size = hdr->total_length;
                last_fragment.fragments.insert( { fragment_off, pkt });

                kMsgReassemblyMap.insert({ hdr->identification, last_fragment });
                kMsgRessemblyLock.store(0, std::memory_order_release);  // 释放锁
                return NET_ERR_OK;
            }
            else   // 当前不是第一片 
            {
                it->second.recv_size += hdr->total_length;
                if (last_fragment.total_size != 0)
                    it->second.total_size = last_fragment.total_size;

                it->second.fragments.insert( { fragment_off, pkt });
                if (it->second.recv_size >= it->second.total_size)
                {
                    complete_msg = it->second;
                    kMsgReassemblyMap.erase(it);
                    kMsgRessemblyLock.store(0, std::memory_order_release);  // 释放锁
                }
                else 
                {
                    kMsgRessemblyLock.store(0, std::memory_order_release);  // 释放锁
                    return NET_ERR_OK;
                }
            }
        }
        
        if (complete_msg.recv_size != 0)    // 表示已经接收到一个完整数据报了
        {
        // 进行分片重组并把每个小分片内存释放
            for (auto& fragment : complete_msg.fragments)
            {
                pkt->Merge(*fragment.second.get());
                pkt.reset();
            }
        }

        switch (hdr->protocol)
        {
            case TYPE_ICMP:
                IcmpPop(pkt);
                break;
            case TYPE_TCP:
                TcpPop(pkt);
                break;
            case TYPE_UDP:
                UdpPop(pkt);
                break;
            default:
                pkt.reset();
                break;
        }

        return NET_ERR_OK;
    }
}