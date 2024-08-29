#include "ipv4.h"
#include "ether.h"
#include "icmp.h"
#include "ipaddr.h"
#include "net.h"
#include "net_type.h"
#include "packet_buffer.h"
#include <arpa/inet.h>
#include <memory>


namespace netstack 
{
    static uint16_t kDataIdentification = 0;    // 数据包的标识
    static uint16_t kDefaultTTL = 64;           // 默认TTL为64(linux默认为这个值)

    /*
        计算方法： 校验和设置为0,首部所有字段反码求和然后结果再次反码然后存入到首部校验和里面.  
        判断方法: 求反码求和再取反码,如果结果为0则正确,否则丢弃
    */

    /*
        问题: 为什么使用反码计算校验和?
            为什么每个路由器都要重新计算首部校验和?

        回答:
            问题1: 容错性好, 反码对于传输过程中比特翻转能够较好的检测出来.
            处理溢出简单，如果有溢出只需将溢出的部分加回到结果中,适合在硬件中实现

            问题2: 当IP数据报经过路由器,会更新TTL字段,还有一些其他字段,所以首部校验和必须更新
            错误检查
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


    void Ipv4Network2Host(IPV4_Hdr* hdr)
    {
        hdr->total_length = ntohl(hdr->total_length);
        hdr->identification = ntohl(hdr->identification);
        hdr->flags_fragment = ntohs(hdr->flags_fragment);
        hdr->src_ipaddr = ntohl(hdr->src_ipaddr);
        hdr->dst_ipaddr = ntohl(hdr->dst_ipaddr);
    }

    void IPv4Push(std::shared_ptr<PacketBuffer> pkt, uint8_t* src_ip, uint8_t* dst_ip, PROTO_TYPE type)
    {
        IPV4_Hdr hdr;
        hdr.version = 4;    
        hdr.header_length = 5;  // 没有选项,默认头部为20字节
        hdr.ds = DSCP_CS0;      // 尽力而为
        hdr.ecn = 0;
        
        size_t data_size = pkt->TotalSize();
        if (data_size > 1480)   // 需要分片
        {
            // TODO: ip分组
        }
        hdr.total_length = data_size;
        hdr.identification = kDataIdentification++;
        hdr.flags = 4;  // 1 0 0: 禁止分片、没有更多分片、保留位为0
        hdr.fragment_offset = 0;
        hdr.ttl = kDefaultTTL;
        hdr.protocol = type;
        hdr.head_checksum = 0;
        hdr.src_ipaddr = *(uint32_t*)src_ip;
        hdr.dst_ipaddr = *(uint32_t*)dst_ip;
        Ipv4Host2Network(&hdr); // 将头部转换成网络字节序

    // 计算头部校验和
        hdr.head_checksum = CheckSum(&hdr);
        hdr.head_checksum = htons(hdr.head_checksum);
        
        IPV4_Hdr* hdr_ptr = pkt->AllocateObject<IPV4_Hdr>();
        *hdr_ptr = hdr;

        // TODO: 计算出应该发往哪里

        EtherPush(pkt, TYPE_IPV4);  // 交给以太网模块去发送
    }



    void IPv4Pop(std::shared_ptr<PacketBuffer> pkt)
    {
        IPV4_Hdr* hdr = pkt->GetObjectPtr<IPV4_Hdr>();
        if (CheckSum(hdr) != 0) // 这个数据包有错误
        {
            pkt.reset();
            return;
        }

        Ipv4Host2Network(hdr);
    // 检查是否是 xxx.xxx.xxx.255 或者255.255.255.255这样的广播地址
    // 如果是广播地址要进行处理
        std::string ip_str = Ip2Str((uint8_t*)&hdr->dst_ipaddr);
        std::string last_str = ip_str.substr(ip_str.size() - 3, ip_str.size());
        NetInfo* info = nullptr;
        if (last_str != "255")
        {
        // 然后再进行匹对是否目的地址是自己
            info = NetInit::GetInstance()->GetNetworkInfo(hdr->dst_ipaddr);
            if (info == nullptr)    // 不是发给当前主机的ip数据包则丢弃掉
            {
                pkt.reset();
                return;
            }
        }
        else 
        {
            // TODO: 进行掩码匹配判断是否是一个子网的

        }

        uint8_t protocol = hdr->protocol;
        pkt->RemoveHeader(sizeof(IPV4_Hdr));

        switch (protocol) 
        {
            case TYPE_TCP:

                break;
            case TYPE_UDP:

                break;
            case TYPE_ICMP:
                IcmpPop(pkt);   // 交给icmp模块去处理
                break;
            default:
                pkt.reset();    // 其他协议的不处理
                break;
        }
    }
}