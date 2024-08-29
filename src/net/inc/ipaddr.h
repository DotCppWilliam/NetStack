#pragma once

#include "net_err.h"
#include <cstdint>
#include <string>
#include <ifaddrs.h>

namespace netstack 
{
    #define IPV4_ADDR_SIZE  4
    enum AddrType {
        IPADDR_V4,
    };

    struct IpAddr
    {
        IpAddr& operator=(IpAddr& rhs)
        {
            if (&rhs == this)
                return *this;
            
            this->addr32_ = rhs.addr32_;
            this->type_ = rhs.type_;
            return *this;
        }
        union {
            uint32_t addr32_ = 0;
            uint8_t addr8_arr_[IPV4_ADDR_SIZE];
        };
        AddrType type_ = IPADDR_V4;
    };

    /**
     * @brief 将IpAddr结构转换成字符串
     * 
     * @param src_str 
     * @param dst_addr 
     * @return NetErr_t 
     */
    NetErr_t IpAddr2Str(IpAddr src_addr, std::string& dst_str);

    std::string Ip2Str(uint8_t ip[4]);
    
    void IpStr2Ip(std::string ip_str, uint8_t ip[4]);

    void MacStr2Mac(std::string mac_str, uint8_t mac[6]);

    std::string Sockaddr2str(struct ifaddrs* addr);

    std::string Mac2Str(struct ifaddrs* ifa);

    std::string Mac2Str(uint8_t mac[6]);


}