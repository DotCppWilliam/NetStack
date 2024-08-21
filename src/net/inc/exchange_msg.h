#pragma once

#include "auto_lock.h"
#include "packet_buffer.h"
#include "sys_plat.h"
#include <deque>
#include <memory>

namespace netstack 
{
    class NetInterface;
    enum MsgType 
    {
        MSG_TYPE_SEND_PKT,  // 发包消息
        MSG_TYPE_RECV_PKT   // 收包消息
    };

    struct PktMsg
    {
        PktMsg(NetInterface* nif, MsgType msg_type)
            : netif(nif), type(msg_type) {}

        NetInterface*   netif = nullptr;                // 哪一个网卡
        MsgType         type;                           // 什么消息
        std::shared_ptr<PacketBuffer> pkt = nullptr;    // 数据包,可以为空
    };

    // 消息队列
    class ExchangeMsg
    {
    public:
        ExchangeMsg();
        ~ExchangeMsg();

        void SendMsg(NetInterface* netif, MsgType pkt_type);
    private:
        void WorkThreadFunc(void* arg);
    private:
        std::deque<PktMsg> msg_deque_;  // 消息队列
        Lock lock_;
        Thread work_thread_;    // 工作线程
    };
}