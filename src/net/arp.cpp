#include "arp.h"
#include "ether.h"
#include "net_err.h"
#include "net_interface.h"
#include "packet_buffer.h"
#include "net_type.h"
#include "sys_plat.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <cstring>
#include <mutex>
#include <pthread.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <chrono>
#include <map>


// 知道目标ip地址,但是不知道mac地址.需要将目标ip地址设置为广播地址,然后发送ARP请求
namespace netstack
{
    extern std::map<uint32_t, NetInterface*> kNetifacesMap;   // 定义在net_init.cpp



    struct WaitArpReplyVal
    {
        std::atomic<bool> flag = false;
        uint64_t mac[6];
    };
    struct WaitArpRelpyKey
    {
        bool operator<(const WaitArpRelpyKey& rhs) const
        {
            if (src_ip != rhs.src_ip)
                return src_ip < rhs.src_ip;
            return dst_ip < rhs.dst_ip;
        }
        uint32_t src_ip;
        uint32_t dst_ip;
    };
    // key: 源ip、目标ip       value: 原子操作的状态值、mac地址
    std::map<WaitArpRelpyKey, WaitArpReplyVal*> kWaitArpReplyMap;
    std::mutex kWaitArpReplyMutex; // 用于上面的锁
    using WaitArpReply = std::map<WaitArpRelpyKey, WaitArpReplyVal*>;





////////////////////////////////////////////////////////////////// 函数
    bool IsFreeArp(Arp* arp)
    {
        for (int i = 0; i < 6; i++)
        {
            if (arp->dst_hwaddr[i] != 0xFF)
                return false;
        }

        return true;
    }


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
    // 目标mac地址清零
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


    /**
     * @brief 给定一个ip地址,判断是否当前系统的网卡处于统一子网,如果是则返回对于网卡接口
     * 
     * @param dst_ip 
     * @return NetInterface* 
     */
    NetInterface* MatchSubnet(uint8_t dst_ip[4])
    {
        NetInterface* netiface = nullptr;

        uint8_t net_sub_addr[4];    // 网卡的子网地址
        uint8_t net_dst_addr[4];    // 目标ip的子网地址 (两者都需要和子网掩码相与获得)
        for (auto& netif : kNetifacesMap)
        {
            NetInfo* netinfo = netif.second->GetNetInfo();
            
            for (int i = 0; i < 4; i++)
            {
                net_sub_addr[i] = netinfo->ip[i] & netinfo->netmask[i];
                net_dst_addr[i] = dst_ip[i] & netinfo->netmask[i];
            }

            if (*(uint32_t*)net_sub_addr == *(uint32_t*)net_dst_addr)
            {
                netiface = netif.second;
                break;
            }
        }

        return netiface;
    }


    // 等待arp响应包
    NetErr_t HandleWaitArpReply(WaitArpRelpyKey key, WaitArpReplyVal* wait_val, uint8_t* out_mac)
    {
        auto start = std::chrono::steady_clock::now();
        
        // 这块使用自旋,因为获取arp响应包都是局域网,速度是很快的.
        // 如果这个时候让出CPU转而去睡眠,那么回头再被调度来执行,上下文切换会很浪费性能
        while (!wait_val->flag.load(std::memory_order_acquire))
        {
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = now - start;
            if (elapsed.count() > 1)    // 如果过了一秒还是没有收到arp响应包,那网络状态肯定有问题,则直接返回
            {
                std::unique_lock<std::mutex> lock(kWaitArpReplyMutex);
                kWaitArpReplyMap.erase(key);

                delete wait_val;
                return NET_ERR_WAIT_ARP_TIMEOUT;    // 获取arp响应包超时
            }
        }

    // 成功获取arp响应包, 我们在kWaitArpReplyMap添加的项由其他线程删除不用管
        memcpy(out_mac, wait_val->mac, 6);
        delete wait_val;   // 释放堆内存
        
        return NET_ERR_OK;
    }


///////////////////////////////////////////////////////////////// 以下是提供给外面的接口

    /**
     * @brief 提供给上层的接口,获取ip地址对应的mac地址
     * 
     * @param in_src_ip 
     * @param in_need_ip 
     * @param out_dst_mac 
     * @return NetErr_t NET_ERR_OK: 获取成功; NET_ERR_WAIT_ARP_TIMEOUT: 超时
     */
    NetErr_t ArpPush(uint8_t in_src_ip[4], uint8_t in_need_ip[4], uint8_t out_dst_mac[6])
    {
        NetInterface* netif = MatchSubnet(in_need_ip);
        if (netif == nullptr)
            return NET_ERR_DIFF_SUBNET; // 不是同一子网无法向局域网获取该ip地址的mac地址
        
        ArpCache cache;
        {
            pthread_t old = 0;
            pthread_t self = pthread_self();
            while (!kArpAtomicFlag.compare_exchange_strong(old, self, std::memory_order_acquire));

            auto ret = kArpCaches.find(*(uint32_t*)in_need_ip);
            if (ret != kArpCaches.end())
                cache = ret->second;

            kArpAtomicFlag.store(0, std::memory_order_release); // 释放自旋锁
        }

        if (cache.is_invalid)    // 找不到这个ip的mac地址
        {
            // 需要发送ARP请求包获取局域网内某个ip的mac地址
            std::shared_ptr<PacketBuffer> pkt = std::make_shared<PacketBuffer>(sizeof(Arp));
            Arp* arp = pkt->GetObjectPtr<Arp>();

            MakeRequstArp(arp, in_src_ip, kNetifacesMap[*(uint32_t*)in_src_ip]->GetNetInfo()->mac, 
                in_need_ip);
            ArpHost2Network(arp);   // 转换成网络字节序

        // 添加一个arp响应包的等待请求
            WaitArpRelpyKey key;
            WaitArpReplyVal* value = new WaitArpReplyVal;
            {
                std::unique_lock<std::mutex> lock(kWaitArpReplyMutex); 
                kWaitArpReplyMap[key] = value;
            }
        // 发送arp请求包
            EtherPush(pkt, TYPE_ARP, kNetifacesMap[*(uint32_t*)in_src_ip], netif->GetNetInfo());
        
        // 当前线程等待arp响应包,有小概率会超时,没有等待成功
            return HandleWaitArpReply(key, value, out_dst_mac);
        }
        else 
            memcpy(out_dst_mac, cache.mac, 6);
        
        return NET_ERR_OK;
    }




    void HandleFreeArp(Arp* arp)
    {

    }

    void HandleArpRequest(Arp* arp)
    {
        auto it = kNetifacesMap.find(*(uint32_t*)arp->dst_ipaddr);
        if (it == kNetifacesMap.end())  // 不是请求自己的
            return;

        std::shared_ptr<PacketBuffer> pkt = std::make_shared<PacketBuffer>(sizeof(Arp));
        Arp* req_arp = pkt->GetObjectPtr<Arp>();
        MakeRequstArp(req_arp, arp->dst_ipaddr, arp->dst_hwaddr, arp->src_ipaddr);
        ArpHost2Network(req_arp);
        NetInfo* dst_iface_info = MatchSubnet(arp->src_ipaddr)->GetNetInfo();
        EtherPush(pkt, TYPE_ARP, it->second, dst_iface_info);
    }

    void HandleArpReply(Arp* arp)
    {
        // arp响应
        WaitArpReply::iterator it;
        {
        // 检查是否有等待当前arp响应包的线程,如果有及时处理
            std::unique_lock<std::mutex> lock(kWaitArpReplyMutex);
            it = std::find_if(kWaitArpReplyMap.begin(), kWaitArpReplyMap.end(),
            [arp](std::pair<WaitArpRelpyKey, WaitArpReplyVal*> p) {
                return p.first.src_ip == *(uint32_t*)arp->dst_ipaddr 
                    && p.first.dst_ip == *(uint32_t*)arp->src_ipaddr;
            });
            
            if (it != kWaitArpReplyMap.end())   // 表示有线程正在等待这个arp响应包
                memcpy(it->second->mac, arp->src_hwaddr, 6);
            WaitArpReplyVal* val = it->second;
            kWaitArpReplyMap.erase(it);
            lock.unlock();

            val->flag.store(true, std::memory_order_release);
        }
        
        pthread_t old = 0;
        pthread_t self = pthread_self();
    // 获取自旋锁
        while (!kArpAtomicFlag.compare_exchange_strong(old, self));
    
    // 设置arp缓存表
        ArpCache cache;
        cache.SetMac(arp->src_hwaddr);
        kArpCaches[*(uint32_t*)arp->src_ipaddr] = cache;
    // 释放锁
        kArpAtomicFlag.store(0, std::memory_order_release);
    }
    
    /**
     * @brief 
     * 
     * @param pkt 
     * @return NetErr_t 
     */
    NetErr_t ArpPop(std::shared_ptr<PacketBuffer> pkt)
    {
        Arp* arp = pkt->GetObjectPtr<Arp>();
        ArpNetwork2Host(arp);
        
        if (IsFreeArp(arp))         // 免费arp
            HandleFreeArp(arp);
        else if (arp->op_code == ARP_REQUEST)
            HandleArpRequest(arp);  // 处理arp请求
        else if (arp->op_code == ARP_REPLAY)
            HandleArpReply(arp);    // 处理arp响应

        pkt.reset();
        return NET_ERR_OK;
    }
}