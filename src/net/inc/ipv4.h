#pragma once
/*
    0         4         8        16    19   24        31
    | version | 首部长度 | 区分服务 |      总长度         |     -----
    |         标识                | 标志 |    片偏移    |        
    |     生存时间       |  协议   |      首部校验和     |      固定部分(20字节)     
    |                       源地址                    |        
    |                     目的地址                    |      ------
    |               可选字段(长度可变)       |   填充   |
    |                数据部分                         | 
   
                        IP 数据包协议图

    |   首部   |      数据部分       |
    ^
    |
    |
    发送在前
 */
#include "net_type.h"
#include "packet_buffer.h"
#include <cstdint>
#include <map>
#include <memory>


namespace netstack 
{

    enum Dscp // 用于ipv4头部中区分服务字段(DS和ECN)
    {
        DSCP_CS0    = 0x000000,     // 类别选择(尽力而为/常规)
        DSCP_CS1    = 0x001000,     // 类别选择(优先)
        DSCP_CS2    = 0x010000,     // 类别选择(立即)
        DSCP_CS3    = 0x011000,     // 类别选择(瞬间)
        DSCP_CS4    = 0x100000,     // 类别选择(瞬间覆盖)
        DSCP_CS5    = 0x101000,     // 类别选择(CRITIC/ECP)
        DSCP_CS6    = 0x110000,     // 类别选择(网间控制)
        DSCP_CS7    = 0x111000,     // 类别选择(控制)

        DSCP_AF11   = 0x001010,     // 保证转发(1, 1)
        DSCP_AF12   = 0x001100,     // 保证转发(1, 2)
        DSCP_AF13   = 0x001110,     // 保证转发(1, 3)
        DSCP_AF21   = 0x010010,     // 保证转发(2, 1)
        DSCP_AF22   = 0x010100,     // 保证转发(2, 2)
        DSCP_AF23   = 0x010110,     // 保证转发(2, 3)
        DSCP_AF31   = 0x011010,     // 保证转发(3, 1)
        DSCP_AF32   = 0x011100,     // 保证转发(3, 2)
        DSCP_AF33   = 0x011110,     // 保证转发(3, 3)
        DSCP_AF41   = 0x100010,     // 保证转发(4, 1)
        DSCP_AF42   = 0x100100,     // 保证转发(4, 2)
        DSCP_AF43   = 0x100110,     // 保证转发(4, 3)
        DSCP_EF_PHB = 0x101110,     // 加速转发
        DSCP_VOICE_ADMIT = 0x101100 // 容量许可的流量
    };

    // 分片标志(一共3位)
    enum FragmentFlag
    {
        FRAG_NO_SHARD           = 2,    // 禁止分片 (010)
        FRAG_MORE_FRAGMENT      = 1,    // 允许分片,还有更多分片 (001)
        FRAG_NO_MORE_FRAGMENT   = 0     // 允许分片,没有更多分片 (000)
    };

    #pragma pack(1)
    struct IPV4_Hdr
    {
        union {
            uint8_t     version             : 4;    // 版本
            uint8_t     header_length       : 4;    // 头部长度(表示头部可以有多少个 (4) 字节[最多15个])
            uint8_t     version_length;
        };

        union {
            /*
            DS字段: 
            优先级(3位,数值越大优先级越高)   D(低延迟) T(高吞吐量) R(高可靠性.这些分别设置为1)
            */
            uint8_t     ds                  : 6;    // 区分服务字段
            // ECN字段: [DS5 DS4 DS3 DS2 DS1 DS0(默认为0)]
            //          DS5、4、3 表示类型; DS2、1表示丢弃概率
            // 前三个表示流量类别(数值越大处理优先级越高), 后三个表示丢弃优先级(数值越大丢弃优先级越高)
            uint8_t     ecn                 : 2;    // 显式拥塞通知
            uint8_t     service;
        };
        
        uint16_t    total_length;               // 总长度(数据长度,包括头部***)
        uint16_t    identification;             // 标识

        union {
            // 标志位( 第一位保留位, 第二位[DF禁止分片: 0:允许分片 1:禁止分片], 
            // 第三位[MF更多分片, 0: 最后一个分片 1:后面还有分片])
            uint8_t     flags               : 3;    
            uint16_t    fragment_offset     : 13;   // 分片偏移, 单位(8字节)
            uint16_t    flags_fragment;
        } ;

        uint8_t     ttl;                        // 生存时间
        uint8_t     protocol;                   // 协议
        uint16_t    head_checksum;              // 头部校验和
    
        uint32_t    src_ipaddr;                 // 源ip地址
        uint32_t    dst_ipaddr;                 // 目标ip地址
    };
    #pragma pack()

    #pragma pack(1)
    struct IPV4_ExtendHdr
    {
        void* options;  // 选项
        void* data;     // 数据
    };
    #pragma pack()



    #pragma pack(1)
    struct MsgReassembly    // 报文重组
    {
        uint32_t recv_size; // 已收到的分片大小(只累加其中的数据大小)
        // key: 分片偏移,   value: 接收到的分片
        std::map<uint16_t, std::shared_ptr<PacketBuffer>> fragments;
    };
    #pragma pack()

    void IPv4Push(std::shared_ptr<PacketBuffer> pkt, uint8_t* src_ip, uint8_t* dst_ip, PROTO_TYPE type);
    void IPv4Pop(std::shared_ptr<PacketBuffer> pkt);

}