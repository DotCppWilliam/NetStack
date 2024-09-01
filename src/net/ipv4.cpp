#include "ipv4.h"
#include "ether.h"
#include "net.h"
#include "net_interface.h"
#include "net_type.h"
#include "packet_buffer.h"
#include "util.h"

#include <arpa/inet.h>
#include <memory>


#define IPV4_DATA_MAX_SIZE  (1480)      // ipv4数据载荷最大支持1480字节: MTU(1500) - ipv4头部(20字节)

namespace netstack 
{
    extern std::list<NetInterface*> kNetifLists;

    static uint16_t kDataIdentification = 0;    // 数据包的标识
    static uint16_t kDefaultTTL = 64;           // 默认TTL为64(linux默认为这个值)

    // key: 标识,   value: 收到的报文
    static std::map<uint16_t, MsgReassembly> kMsgReassemblyMap; // 接收重组分片的map


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

    // 将ipv4头转成网络字节序
    void Ipv4Host2Network(IPV4_Hdr* hdr)
    {
        hdr->total_length = htonl(hdr->total_length);
        hdr->identification = htonl(hdr->identification);
        hdr->flags_fragment = htons(hdr->flags_fragment);
        hdr->src_ipaddr = htonl(hdr->src_ipaddr);
        hdr->dst_ipaddr = htonl(hdr->dst_ipaddr);
    }

    /**
     * @brief 将ipv4头部转换成网络字节序
     * 
     * @param hdr 
     */
    void Ipv4Network2Host(IPV4_Hdr* hdr)
    {
        hdr->total_length = ntohl(hdr->total_length);
        hdr->identification = ntohl(hdr->identification);
        hdr->flags_fragment = ntohs(hdr->flags_fragment);
        hdr->src_ipaddr = ntohl(hdr->src_ipaddr);
        hdr->dst_ipaddr = ntohl(hdr->dst_ipaddr);
    }

    /**
     * @brief 数据报分片
     * 
     * @param pkt 
     */
    void Ipv4Fragment(std::shared_ptr<PacketBuffer> pkt, IPV4_Hdr hdr)
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
            
            pkt->AddHeader(sizeof(IPV4_Hdr), (const unsigned char*)&hdr);
            Ipv4Host2Network(&hdr);

            EtherPush(chunk_pkt, TYPE_IPV4);    
        }
        pkt.reset();
    }


    /**
     * @brief 提供给上层传输层使用,比如UDP、TCP、ICMP.
     *        给上层数据增加ipv4头,如果数据包比较大则进行分片
     * 
     * @param pkt 上层的数据包
     * @param src_ip 源ip地址,可以为空表示任意源地址
     * @param dst_ip 目标地址
     * @param type 上层使用的是什么协议
     */
    void IPv4Push(std::shared_ptr<PacketBuffer> pkt, uint8_t* src_ip, uint8_t* dst_ip, PROTO_TYPE type)
    {
    // 设置ipv4头
        IPV4_Hdr hdr;
        hdr.version = 4;
        hdr.header_length = 5;
        hdr.ds = DSCP_CS0;
        hdr.ecn = 0;
        hdr.identification = GetRandomNum();
        hdr.ttl = kDefaultTTL;
        hdr.protocol = type;
        hdr.dst_ipaddr = *(uint32_t*)dst_ip;

        if (src_ip == nullptr)
        {
            auto it = std::find_if(kNetifLists.begin(), kNetifLists.end(),
            [](NetInterface* iface){
                return iface->IsDefaultGetaway();
            });
            hdr.src_ipaddr = *(uint32_t*)(*it)->GetNetInfo()->ip;
        }
        else 
            hdr.src_ipaddr = *(uint32_t*)src_ip;
        
        // 进行分片
        if (pkt->TotalSize() > IPV4_DATA_MAX_SIZE)  
        {
            Ipv4Fragment(pkt, hdr);
            return;
        }
        
        // 不分片
        hdr.total_length = pkt->DataSize() + 20;
        hdr.fragment_offset = 0;
        hdr.flags = FRAG_NO_SHARD;
        hdr.head_checksum = CheckSum(&hdr);

        pkt->AddHeader(sizeof(hdr), (const unsigned char*)&hdr);
        EtherPush(pkt, TYPE_IPV4);
    }



    void IPv4Pop(std::shared_ptr<PacketBuffer> pkt)
    {
        IPV4_Hdr* hdr = pkt->GetObjectPtr<IPV4_Hdr>();
        


    }
}