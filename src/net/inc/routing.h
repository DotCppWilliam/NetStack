#pragma once

#include "net_interface.h"
#include <cstdint>

namespace netstack
{
    /*
        网络目标: 与掩码相与的结果用于定义本地计算机可以到达的目的网络的地址范围(是否是一个子网)
        网络掩码: 和上述相同
        网关: 又称为下一跳服务器.在发送IP数据报时,网关定义了针对特定的网络目的地址,发送到下一跳服务器
        接口: 针对特定的网络目的地址,本地计算机用于发送数据报的网络接口
        跃点数: 用于指出路由的成本,通常情况下代表目标地址所需经过的跃点数量
        flags: 表示需要怎么走

        目的地址为0.0.0.0为缺省路由,目的网段不在路由表时,发送该表项对应的网关,间接交付
    */

    /*
        路由表项 分类:
        1. 固定路由(静态路由): 0.0.0.0、
        2. 本地网络路由: 
        3. 主机路由
        4. 回环接口路由: 
        5. 广播地址路由:
        6. 多播地址路由:
    */
    
    enum RoutingFlag 
    {
        ROUTE_DEFAULT_GATEWAY   = 1,
        ROUTE_DIRECT_SEND       = 2,
        ROUTE_INDIRECT_SEND     = 4
    };

    #pragma pack(1)
    struct Routing 
    {
        Routing();
        ~Routing() = default;
        Routing(const Routing& rhs);
        Routing& operator=(const Routing& rhs);

        uint32_t        ip_;        // 目的地址
        uint32_t        gateway_;   // 网关
        uint32_t        netmask_;   // 掩码
        RoutingFlag     flag_;      // 标志
        long            metric_;    // 跃点数,这里表示耗时时间
        NetInterface*   iface_;     // 接口
    };
    #pragma pack()

    Routing GetRouting(uint32_t ip);

    void InitRoutingMap();
}