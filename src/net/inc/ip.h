#pragma once
/*
    0         4         8        16    19   24        31
    | version | 首部长度 | 区分服务 |      总长度         |     -----
    |         标识                | 标志 |    片偏移    |        |
    |     生存时间       |  协议   |      首部校验和     |      固定部分(20字节)     
    |                       源地址                    |        |
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
#include <cstdint>


namespace netstack 
{
    #pragma pack(1)
    struct IPV4_Hdr
    {
        uint8_t     version             : 4;    // 版本
        uint8_t     header_length       : 4;    // 头部长度
        uint8_t     ds                  : 6;    // 区分服务字段
        uint8_t     ecn                 : 2;    // 显式拥塞通知
        uint16_t    total_length;               // 总长度

        uint16_t    identification;             // 标识
        uint8_t     flags               : 3;    // 标志位
        uint16_t    fragment_offset     : 13;   // 分片偏移

        uint8_t     ttl;                        // 生存时间
        uint8_t     protocol;                   // 协议
        uint16_t    head_checksum;              // 头部校验和
    
        uint32_t    src_ipaddr;                 // 源ip地址
        uint32_t    dst_ipaddr;                 // 目标ip地址
    // 从这往上是20字节

        void*       options = nullptr;          // 选项, 可变长度,最多320位即40字节
        void*       data = nullptr;             // 数据, 可变长度,最多524120位即65515字节
    };
    #pragma pack()


    void IPv4Push();
    void IPv4Pop();
}