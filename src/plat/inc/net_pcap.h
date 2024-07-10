#pragma once

#include "../../net/inc/net_err.h"
#include "sys_plat.h"

namespace lpcap 
{
    net::NetErr_t NetIfPcapOpen();

    class NetifPcap 
    {
    public:
        NetifPcap() :
            recv_thread_(&NetifPcap::SendThread, nullptr),
             send_thread_(&NetifPcap::RecvThread, nullptr) {}

        static void SendThread(void* arg);
        static void RecvThread(void* arg);
        net::NetErr_t Open();
    private:
        SysThread recv_thread_;
        SysThread send_thread_;
    };
}