#pragma once

#include "auto_lock.h"
#include "net_err.h"
#include "sys_plat.h"
#include <deque>

namespace netstack 
{
    class NetInterface;

    // 消息队列
    class ExchangeMsg
    {
        struct PktMsg
        {
            PktMsg(NetInterface* nif, bool is_recv_pkt)
                : netif(nif), recv_pkt(is_recv_pkt) {}

            NetInterface* netif = nullptr;
            bool recv_pkt = false;
        };
    public:
        ExchangeMsg();
        ~ExchangeMsg();

        void SendMsg(NetInterface* netif, bool recv_pkt);
    private:
        void WorkThreadFunc(void* arg);
    private:
        std::deque<PktMsg> msg_deque_;  // 消息队列
        Lock lock_;
        Thread work_thread_;    // 工作线程
    };
}