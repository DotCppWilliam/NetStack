#include "routing.h"
#include "net_err.h"
#include "net_init.h"
#include "arp.h"
#include "net_interface.h"

namespace netstack 
{
    extern std::map<uint32_t, NetInterface*> kNetifacesMap;    // 定义在net_init.cpp中
    static std::map<uint32_t, Routing> kRoutingMap;    // 路由表
    static Routing kDefGateway;


    Routing::Routing()
        : ip_(0), gateway_(0),
        netmask_(0), flag_((RoutingFlag)0),
        metric_(0), iface_(nullptr)
    {

    }

    Routing::Routing(const Routing& rhs)
    {
        ip_ = rhs.ip_;
        gateway_ = rhs.gateway_;
        netmask_ = rhs.netmask_;
        flag_ = rhs.flag_;
        metric_ = rhs.metric_;
        iface_ = rhs.iface_;
    }

    Routing& Routing::operator=(const Routing& rhs)
    {
        if (this == &rhs)
            return *this;
        ip_ = rhs.ip_;
        gateway_ = rhs.gateway_;
        netmask_ = rhs.netmask_;
        flag_ = rhs.flag_;
        metric_ = rhs.metric_;
        iface_ = rhs.iface_;
        return *this;
    }





    /**
     * @brief 初始化路由表
     * 
     */
    void InitRoutingMap()
    {
        NetErr_t ret;
        uint8_t out_dst_mac[6];

    // 初始化默认路由
        NetInterface* def_gateway = nullptr;
        uint8_t subnet_addr[4];
        for (auto& netif : kNetifacesMap)
        {
            NetInfo* info = netif.second->GetNetInfo();
            if (info->is_default_gateway_)
                continue;
            for (int i = 0; i < 4; i++)
                subnet_addr[i] = info->ip[i] & info->netmask[i];
            subnet_addr[3] = 1;

            ret = ArpPush(info->ip, subnet_addr, out_dst_mac, &kDefGateway.metric_);
            if (ret == NET_ERR_OK)
            {
                def_gateway = netif.second;
                break;
            }
        }
        
        if (ret == NET_ERR_OK)
        {
            kDefGateway.ip_ = 0;
            kDefGateway.gateway_ = *(uint32_t*)subnet_addr;
            kDefGateway.netmask_ = *(uint32_t*)def_gateway->GetNetInfo()->netmask;
            kDefGateway.flag_ = RoutingFlag(ROUTE_DEFAULT_GATEWAY | ROUTE_DIRECT_SEND);
            kDefGateway.iface_ = def_gateway;
            kRoutingMap.insert({0, kDefGateway});   // 插入默认路由
        }
    }

    Routing GetRouting(uint32_t ip)
    {
        Routing routing;
        NetInterface* netif = MatchSubnet((uint8_t*)&ip);
        if (netif == nullptr)
            return kDefGateway; // 返回默认路由,需要间接转发

        routing.iface_ = netif;
        routing.ip_ = ip;
        routing.flag_ = (RoutingFlag)ROUTE_INDIRECT_SEND;
        kRoutingMap.insert( { ip, routing });

        return routing;
    }
}