#pragma once

/*
    ARP协议:

    大小       2         2        1         1       2          6               4              6              4
         | 硬件类型 | 协议类型 | 硬件大小 | 协议大小 | 操作码 | 发送方硬件地址 | 发送方IPV4地址 | 目的方硬件地址 | 目的方IPV4地址 |
 */

#include "packet_buffer.h"
#include "time_entry.h"
#include "net_err.h"

#include <cstdint>
#include <memory>
#include <atomic>
#include <map>
#include <cstring>

namespace netstack
{   
    #define ARP_HW_ETHER    1   // 硬件类型
    #define ARP_REQUEST     1   // 协议类型
    #define ARP_REPLAY      2   

    #define ARP_TIMEOUT     (60)    // ARP缓存表过期时间为60秒

    #pragma pack(1)
    struct ArpCache
    {
        ArpCache() 
            : timeout( { ARP_TIMEOUT, 0 } ) {}
        void SetMac(uint8_t* mac_param)
        { 
            memcpy(mac, mac_param, 6); 
            is_invalid = false;
        }

        uint8_t mac[6];
        TimeEntry timeout;          // 超时时间, 默认超时时间为 60秒
        bool is_expired = false;    // 是否过期
        bool is_invalid = true;     // 默认为无效的
    };
    #pragma pack()

    // ARP缓存表
    static std::map<uint32_t, ArpCache> kArpCaches;  
    static std::atomic<pthread_t> kArpAtomicFlag;


    #pragma pack(1)
    struct Arp 
    {
        Arp(const Arp& rhs)
        {
            hw_type = rhs.hw_type;
            proto_type = rhs.proto_type;
            hw_addr_size = rhs.hw_addr_size;
            proto_addr_size = rhs.proto_addr_size;
            op_code = rhs.op_code;
            memcpy(src_hwaddr, rhs.src_hwaddr, sizeof(src_hwaddr));
            memcpy(src_ipaddr, rhs.src_ipaddr, sizeof(src_ipaddr));
            memcpy(dst_hwaddr, rhs.dst_hwaddr, sizeof(dst_hwaddr));
            memcpy(dst_ipaddr, rhs.dst_ipaddr, sizeof(dst_ipaddr));
        }

        Arp& operator=(const Arp& arp)
        {
            if (&arp == this)   
                return *this;

            hw_type = arp.hw_type;
            proto_type = arp.proto_type;
            hw_addr_size = arp.hw_addr_size;
            proto_addr_size = arp.proto_addr_size;
            op_code = arp.op_code;
            memcpy(src_hwaddr, arp.src_hwaddr, sizeof(src_hwaddr));
            memcpy(src_ipaddr, arp.src_ipaddr, sizeof(src_ipaddr));
            memcpy(dst_hwaddr, arp.dst_hwaddr, sizeof(dst_hwaddr));
            memcpy(dst_ipaddr, arp.dst_ipaddr, sizeof(dst_ipaddr));
            return *this;
        }

        uint16_t    hw_type;            // 硬件类型
        uint16_t    proto_type;         // 协议类型
        uint8_t     hw_addr_size;       // 硬件地址大小
        uint8_t     proto_addr_size;    // 协议地址大小
        uint16_t    op_code;            // 操作码
        uint8_t     src_hwaddr[6];      // 发送方硬件地址
        uint8_t     src_ipaddr[4];      // 发送IPV4地址
        uint8_t     dst_hwaddr[6];      // 目的方硬件地址
        uint8_t     dst_ipaddr[4];      // 目的方ipv4地址
    };
    #pragma pack()

    class NetInterface;
    NetInterface* MatchSubnet(uint8_t dst_ip[4], uint32_t* gateway = nullptr);
    /**
     * @brief 提供给上层的接口,获取ip地址对应的mac地址
     * 
     * @param ip 
     * @param mac 
     * @return true 
     * @return false 
     */
    NetErr_t ArpPush(uint8_t in_src_ip[4], uint8_t in_need_ip[4], uint8_t out_dst_mac[6], long* time = nullptr);

    /**
     * @brief 
     * 
     * @param pkt 
     * @return NetErr_t 
     */
    NetErr_t ArpPop(std::shared_ptr<PacketBuffer> pkt);
}