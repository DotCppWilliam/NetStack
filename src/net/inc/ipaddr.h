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
    NetErr_t IpAddr2Str(IpAddr& src_addr, std::string& dst_str);

    /**
     * @brief 将字符串转换成IpAddr结构, 比如192.168.1.1转换成 IpAddr{ 192, 168, 1, 1}这样数组
     * 
     * @param src_addr 
     * @param dst_str 
     * @return NetErr_t 返回NET_ERR_OK表示成功
     */
    NetErr_t Str2IpAddr(std::string& src_str, IpAddr& dst_addr);


    std::string Sockaddr2str(struct ifaddrs* addr);
}