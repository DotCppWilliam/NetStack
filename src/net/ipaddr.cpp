#include "ipaddr.h"
#include "net_err.h"

#include <arpa/inet.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <ifaddrs.h>

namespace netstack 
{
    NetErr_t IpAddr2Str(IpAddr src_addr, std::string& dst_str)
    {
        if (src_addr.addr32_ == 0)
            return NET_ERR_PARAM;
        
        std::string ret;
        char buf[32];
        for (int i = 0; i < 4; i++)
        {
            if (src_addr.addr8_arr_[i] < 0)
                return NET_ERR_PARAM;
            memset(buf, '\0', sizeof(buf));
            snprintf(buf, sizeof(buf), "%d", src_addr.addr8_arr_[i]);
            ret += buf;
            if (i <= 2)
                ret += ".";
        }
        dst_str = ret;
        return NET_ERR_OK;
    }

    std::string Ip2Str(uint8_t ip[4])
    {
        std::string ip_str;
        char buf[32];
        for (int i = 0; i < 4; i++)
        {
            if (ip[i] < 0)
                return std::string();
            memset(buf, '\0', sizeof(buf));
            snprintf(buf, sizeof(buf), "%d", ip[i]);
            ip_str += buf;
            if (i <= 2)
                ip_str += ".";
        }

        return ip_str;
    }

    void IpStr2Ip(std::string ip_str, uint8_t ip[4])
    {
        // TODO:
    }

    void MacStr2Mac(std::string mac_str, uint8_t mac[6])
    {
        // TODO:
    }

    NetErr_t Str2IpAddr(std::string src_str, IpAddr& dst_addr)
    {
        if (src_str.empty())
            return NET_ERR_PARAM;
        
        IpAddr addr;
        std::string tmp;
        std::string::size_type ret;
        int index = 0;

        while ((ret = src_str.find(".")) != std::string::npos)
        {   
            tmp = src_str.substr(0, ret);
            src_str = src_str.substr(ret + 1, src_str.size());
            for (int i = 0; i < tmp.size(); i++)
            {
                if (!isalnum(tmp[i]))
                    return NET_ERR_PARAM;
            }
            if (tmp.size() > 3)
                return NET_ERR_PARAM;

            int num = atoi(tmp.c_str());
            if (num < 0)
                return NET_ERR_PARAM;
            addr.addr8_arr_[index++] = num;
        }
        if (!src_str.empty() && (ret = src_str.find(".")) == std::string::npos)
        {
            int num = atoi(src_str.c_str());
            addr.addr8_arr_[index++] = num;
            src_str.clear();
        }

        if (!src_str.empty() || index != 4)
            return NET_ERR_PARAM;
        
        dst_addr = addr;
        return NET_ERR_OK;
    }



    std::string Sockaddr2str(struct ifaddrs* addr)
    {
        std::string ret;
        struct sockaddr_in* saddr = reinterpret_cast<struct sockaddr_in*>(addr->ifa_addr);
        char buf[256] = "";
        inet_ntop(AF_INET, &saddr->sin_addr, buf, sizeof(buf));
        ret = buf;

        return ret;
    }

    std::string Socknetmask2str(struct ifaddrs* netmask)
    {
        std::string ret;
        struct sockaddr_in* saddr = reinterpret_cast<struct sockaddr_in*>(netmask->ifa_netmask);
        char buf[256] = "";
        inet_ntop(AF_INET, &saddr->sin_addr, buf, sizeof(buf));
        ret = buf;

        return ret;
    }


    void print_mac_address(const unsigned char *mac, std::string& mac_str) 
    {
        char str[512] = "";
        snprintf(str, sizeof(str), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        //printf("MAC地址: %s\n", str);
        mac_str = str;
    }


    std::string Mac2Str(struct ifaddrs* ifa)
    {
        struct sockaddr_ll *s = (struct sockaddr_ll *)ifa->ifa_addr;
        std::string str(256, ' ');
        
        snprintf(str.data(), str.size(), "%02x:%02x:%02x:%02x:%02x:%02x",
            s->sll_addr[0], s->sll_addr[1], s->sll_addr[2], s->sll_addr[3], s->sll_addr[4], s->sll_addr[5]);
        return str;
    }


    std::string Mac2Str(uint8_t mac[6])
    {
        std::string str(256, ' ');
        snprintf(str.data(), str.size(), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return str;
    }


    bool Strmac2Num(std::string mac_str, uint8_t (&arr)[6])
    {
        size_t size = mac_str.size();
        long val;
        std::string num_str;
        std::string::size_type it;
        for (size_t i = 0, index = 0; i < 6; i++)
        {
            if ((it = mac_str.find(":")) != std::string::npos)
            {
                num_str = mac_str.substr(0, it);
                mac_str = mac_str.substr(it + 1, mac_str.size());
                val = strtol(num_str.c_str(), nullptr, 16);
            }
            else 
                val = strtol(mac_str.c_str(), nullptr, 16);
            
            arr[index++] = val;
        }
        return true;
    }
}