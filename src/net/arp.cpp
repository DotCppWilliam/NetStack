#include "arp.h"
#include "ether.h"
#include "net.h"
#include "net_pcap.h"
#include "packet_buffer.h"
#include "sys_plat.h"


#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <cstring>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <unordered_map>

// 知道目标ip地址,但是不知道mac地址.需要将目标ip地址设置为广播地址,然后发送ARP请求
namespace netstack
{
    void SetDefaultArp(Arp* arp)
    {
        arp->hw_type = htons(ARP_HW_ETHER);
        arp->proto_type = htons(TYPE_IPV4);
        arp->hw_addr_size = 6;
        arp->proto_addr_size = 4;
    }


    void ArpHost2Network(Arp* arp)
    {
        arp->hw_type = htonl(arp->hw_type);
        arp->proto_type = htonl(arp->proto_type);
        arp->op_code = htonl(arp->op_code);
    }

    void ArpNetwork2Host(Arp* arp)
    {
        arp->hw_addr_size = ntohl(arp->hw_addr_size);
        arp->proto_type = ntohl(arp->proto_type);
        arp->op_code = ntohl(arp->op_code);
    }

    /**
     * @brief 创建一个arp请求包
     * 
     * @param arp 
     * @param src_ip 由上层传递源ip地址
     * @param src_mac 由上层传递源mac地址
     * @param dst_ip 由上层传递目标mac地址
     */
    void MakeRequstArp(Arp* arp, uint8_t src_ip[4], uint8_t src_mac[6], uint8_t dst_ip[4])
    {
        SetDefaultArp(arp);
        arp->op_code = ARP_REQUEST;

        memcpy(arp->src_ipaddr, src_ip, sizeof(arp->src_ipaddr));

        memcpy(arp->src_hwaddr, src_mac, sizeof(arp->src_hwaddr));

        memset(arp->dst_hwaddr, 0, sizeof(arp->dst_hwaddr));

        memcpy(arp->dst_ipaddr, dst_ip, sizeof(arp->dst_ipaddr));
        ArpHost2Network(arp);
    }

    /**
     * @brief 构建一个arp响应包.从网卡接收一个arp请求包,将里面的目标mac地址填写为
     *        当前网卡的mac地址,如果请求的是当前的ip地址的话
     * @param arp 
     * @return true 
     * @return false 
     */
    bool MakeReplyArp(Arp* arp)
    {
        arp->op_code = htons(ARP_REPLAY);
        
        uint8_t ipaddr[4];
        memcpy(ipaddr, arp->dst_ipaddr, sizeof(ipaddr));
        NetInfo* info = NetInit::GetInstance()->GetNetworkInfo(*(uint32_t*)ipaddr);
        if (info == nullptr)
            return false;

        Arp tmp_arp = *arp;
        memcpy(arp->src_ipaddr, arp->dst_ipaddr, sizeof(arp->src_ipaddr));
        
        //MacStr2Mac(info->mac, arp->src_hwaddr); TODO:

        memcpy(arp->dst_hwaddr, tmp_arp.src_hwaddr, sizeof(arp->dst_hwaddr));
        memcpy(arp->dst_ipaddr, tmp_arp.dst_ipaddr, sizeof(arp->dst_ipaddr));

        return true;
    }


    /**
     * @brief 构建一个"免费"arp包. 用于解决ip冲突
     * 
     * @param arp 
     * @param src_ip 
     * @param src_mac 
     * @return true 
     * @return false 
     */
    void MakeFreeArp(Arp* arp, const char* src_ip, const char* src_mac)
    {   
        
    }







///////////////////////////////////////////////////////////////// 以下是提供给外面的接口

    /**
     * @brief 提供给上层的接口,获取ip地址对应的mac地址
     * 
     * @param ip 
     * @param mac 
     * @return true 
     * @return false 
     */
    bool ArpPush(uint8_t in_src_ip[4], uint8_t in_dst_ip[4], uint8_t out_dst_mac[6])
    {
        std::unordered_map<uint32_t, ArpCache>::iterator ret = kArpCaches.end();
        {
            // 获取一把读锁
            std::shared_lock<std::shared_mutex> read_mutex(kRwMutex);
            ret = kArpCaches.find(*(uint32_t*)in_dst_ip);
        }
        
        if (ret == kArpCaches.end())    // 没有找到
        {
            std::shared_ptr<PacketBuffer> arp_pkt = 
                std::make_shared<PacketBuffer>();
            Arp* arp = arp_pkt->AllocateObject<Arp>();

            NetInfo* src_ip_info = NetInit::GetInstance()->GetNetworkInfo(*(uint32_t*)(in_src_ip));
            MakeRequstArp(arp, in_src_ip, src_ip_info->mac, in_dst_ip);    // 构建一个arp请求包
            
            // 创建一个指针,接收网卡包的线程用来设置
            std::shared_ptr<PacketBuffer>* ptr = new std::shared_ptr<PacketBuffer>;
            NetifPcap* ifpcap = NetInit::GetInstance()->GetMsgWorker();
            ifpcap->SetExpectedArpReply(*(uint32_t*)in_dst_ip, ptr);
            EtherPush(arp_pkt, TYPE_ARP); // 发送给网卡

            std::time_t start = std::time(nullptr);
            bool timeout = false;
            while (!ptr->get())
            {
                std::time_t curr = std::time(nullptr);
                double elasped_seconds = std::difftime(curr, start);
                if (elasped_seconds >= 6.0) // 如果超过6秒没有获取到Arp响应包,那就是局域网没有这个IP地址
                {
                    timeout = true;
                    break;
                }
            }

            // 时间超时没有获取arp响应包
            if (timeout && ptr->get() == nullptr)
            {
                delete ptr;
                return false;
            }

            if (ptr->get())
            {
                // 获取一把写锁
                std::shared_lock<std::shared_mutex> write_lock(kRwMutex);
                Arp* arp_ptr = (*ptr)->GetObjectPtr<Arp>(sizeof(EtherHdr));
                ArpCache cache((uint64_t)arp_ptr->src_hwaddr);
                kArpCaches.insert( { *(uint32_t*)arp_ptr->src_ipaddr, cache });

                memcpy(out_dst_mac, arp_ptr->src_hwaddr, sizeof(arp_ptr->src_hwaddr));
                ptr->reset();
                delete ptr;
                return true;
            }
        }

        return false;
    }


    
    /**
     * @brief 提供给下层(以太网)的接口,用来处理ARP
     * 
     * @param pkt 
     */
    void ArpPop(std::shared_ptr<PacketBuffer> pkt)
    {
        Arp* arp = pkt->GetObjectPtr<Arp>(sizeof(EtherHdr));
        ArpNetwork2Host(arp);
        if (arp->op_code == ARP_REQUEST)    // arp请求包
        {
            if (MakeReplyArp(arp))
            {
                // 将arp响应包发送出去
                EtherPush(pkt, TYPE_ARP);
                return;
            }

            pkt.reset();    // 不是请求当前主机,则释放该报并忽略
            return;
        }

        // 处理ARP响应包
        std::shared_lock<std::shared_mutex> write_lock(kRwMutex);
        ArpCache cache((uint64_t)arp->src_hwaddr);
        kArpCaches.insert( { *(uint32_t*)arp->src_ipaddr, cache});
    }
}