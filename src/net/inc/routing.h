#pragma once

#include "net_interface.h"
#include <cstdint>
#include <map>

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
        ROUTE_UP        = 0x1,  // 该路由是激活的
        ROUTE_GATEWAY   = 0x2,  // 数据报需要通过网关
        ROUTE_HOST      = 0x4,  // 该条目是一个主机路由,而不是网络路由
        ROUTE_REINSTATE = 0x8,  // 动态路由重新加入
        ROUTE_DYNAMIC   = 0x10, // 由路由守护进程或daemon动态安装的
        ROUTE_MODIFIED  = 0x20, // 由路由守护进程或daemon修改的
    };

    #pragma pack(1)
    struct Routing 
    {
        uint32_t        ip_;        // 目的地址
        uint32_t        gateway_;   // 网关
        uint32_t        netmask_;   // 掩码
        RoutingFlag     flag_;      // 标志
        uint32_t        metric_;    // 跃点数
        NetInterface*   iface_;     // 接口
    };
    #pragma pack()


    extern std::map<uint32_t, Routing> kRoutingMap;    // 路由表
}