#pragma once

#include "PacketBuffer.h"
#include "ipaddr.h"
#include "net_err.h"
#include "node.h"
#include "concurrent_queue.h"
#include "sys_plat.h"

#include <string>
#include <memory>

#define DEFAULT_TX_QUEUE_LEN 10000

namespace net 
{
    enum NetIfState
    {
        NETIF_CLOSED,
        NETIF_OPENED,
        NETIF_ACTIVE,
        NETIF_DEACTIVE,
    };


    class NetInterface 
    {
    public:
        NetInterface(void* arg, const char* device_name, int queue_max_threshold = DEFAULT_TX_QUEUE_LEN);
        ~NetInterface();
    public:
        virtual NetErr_t Open(void* arg);
        virtual NetErr_t Close();
        virtual NetErr_t Send();

        void SetDefaultNetif(NetInterface* netif);
        void SetAddr(IpAddr& ip, IpAddr& netmask, IpAddr& gateway);
        void SetMacAddr(std::string& mac_addr);
        NetErr_t SetActiveState();
        NetErr_t SetDeActiveState();
        void DisplayInfo();
        
        NetErr_t Out(IpAddr& addr, std::shared_ptr<PacketBuffer> pkt);

    protected:
        NetErr_t PushPacket(std::shared_ptr<PacketBuffer> pkt, bool is_recv_queue = true, bool wait = false);
        NetErr_t PopPacket(std::shared_ptr<PacketBuffer> pkt, bool is_recv_queue = true, bool wait = false);
    private:
        std::string name_;              // 网卡名称
        std::string mac_addr_;          // MAC地址
        IpAddr ipaddr_;                 // ip地址
        IpAddr netmask_;                // 子网掩码
        IpAddr gateway_;                // 网关
        NetIfType type_;                // 网卡类型: 回环网卡、
        unsigned long long mtu_;        // 网卡传输数据包最大字节
        Node* node_;                    // 指向下一个网卡
        NetIfState state_;              // 当前网卡状态

        util::ConcurrentQueue<std::shared_ptr<PacketBuffer>> recv_queue_;   // 接收数据包队列
        util::ConcurrentQueue<std::shared_ptr<PacketBuffer>> send_queue_;   // 发送数据包队列
        int queue_max_threshold_ = 1000;        // 队列存储数据包最大个数
        void* ops_data_;        // 数据

        static NetInterface* default_netif_;
        static std::list<NetInterface*> netif_lists_;
        lpcap::PcapNICDriver* driver_;  // 网卡驱动: 打开、关闭网卡
    };
}