#pragma once

#include "net_err.h"
#include "net_interface.h"
#include "noncopyable.h"
#include "sys_plat.h"

namespace netstack 
{
    NetErr_t NetIfPcapOpen();

    class NetInterface;

    class NetifPcap : NonCopyable
    {
    public:
        NetifPcap(std::list<NetInterface*>);
        ~NetifPcap();

        void SendThread(void* arg);
        void RecvThread(void* arg);

        /**
         * @brief 打开网卡 Open network device
         * 
         * @return net::NetErr_t 
         */
        NetErr_t OpenDevice();

        static std::list<NetInterface*> kAllNetIf;
    private:
        Thread recv_thread_; // 接收网卡数据的线程
        Thread send_thread_; // 向网卡发送数据的线程
        PcapNICDriver driver;// 默认打开全部网卡
        int recv_epollfd_;
        bool exit_ = false;
    };
}