#pragma once

#include "../../net/inc/net_err.h"
#include "sys_plat.h"

namespace lpcap 
{
    net::NetErr_t NetIfPcapOpen();

    class NetifPcap 
    {
    public:
        NetifPcap() 
        {
            recv_thread_.SetThreadFunc(&NetifPcap::SendThread, this, nullptr);
            send_thread_.SetThreadFunc(&NetifPcap::RecvThread, this, nullptr);
        }
        
        // :
        //     recv_thread_(&NetifPcap::SendThread, nullptr),
        //      send_thread_(&NetifPcap::RecvThread, nullptr) {}

        void SendThread(void* arg);
        void RecvThread(void* arg);

        /**
         * @brief 打开网卡 Open network device
         * 
         * @return net::NetErr_t 
         */
        net::NetErr_t OpenDevice();
    private:
        Thread recv_thread_; // 接收网卡数据的线程
        Thread send_thread_; // 向网卡发送数据的线程
    };
}