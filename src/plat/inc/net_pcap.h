#pragma once

#include "net_err.h"
#include "noncopyable.h"
#include "packet_buffer.h"
#include "sys_plat.h"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace netstack 
{
    NetErr_t NetIfPcapOpen();

    class NetInterface;

    void PrintAll();

    class NetifPcap : NonCopyable
    {
    public:
        NetifPcap();
        ~NetifPcap();

        void SendThread(void* arg);
        void RecvThread(void* arg);

        /**
         * @brief 打开网卡 Open network device
         * 
         * @return net::NetErr_t 
         */
        NetErr_t OpenDevice();

        void SetExpectedArpReply(uint32_t ipaddr, std::shared_ptr<PacketBuffer>* set_ptr);
    private:
        bool IsExpectedArpResponse(std::shared_ptr<PacketBuffer> pkt);
    private:
        CustomThread recv_thread_; // 接收网卡数据的线程
        CustomThread send_thread_; // 向网卡发送数据的线程
        PcapNICDriver driver;// 默认打开全部网卡
        int recv_epollfd_;
        bool exit_ = false;
        std::mutex mutex_;
        std::unordered_map<uint32_t, std::shared_ptr<PacketBuffer>*> arp_reply_set_;   // 存储想要获取的arp包, 存放的想要获取mac地址的ip地址
    };
}