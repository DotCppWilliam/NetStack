#pragma once

#include "packet_buffer.h"
#include "net_err.h"
#include "concurrent_queue.h"
#include "sys_plat.h"

#include <memory>

#define DEFAULT_TX_QUEUE_LEN 1024

namespace netstack 
{
    
    using SharedPkt = std::shared_ptr<PacketBuffer>;
    class NetInterface 
    {
        friend NetInterface* GetLoopNetinterface();
        friend void HandleRecvPktCallback(NetInterface* iface);
        friend void HandleSendPktCallback(NetInterface* iface);
    public:
        NetInterface(NetInfo* netinfo, int queue_max_threshold = DEFAULT_TX_QUEUE_LEN);
        virtual ~NetInterface();
        bool operator<(const NetInterface& rhs) 
        { return netif_fd_ < rhs.netif_fd_; }
    public:
        void SetDefaultNetif(NetInterface* netif);
        void DisplayInfo();
        
        NetInfo* GetNetInfo()
        { return netinfo_; }

        int GetFd() const 
        { return netif_fd_; }

        NetErr_t PushPacket(SharedPkt pkt, bool is_recv_queue = true, bool wait = false);
        NetErr_t PopPacket(SharedPkt& pkt, bool is_recv_queue = true, bool wait = false);

        bool NetRx();   // 从网卡读取数据
        bool NetTx();   // 向网卡写入数据
        NetErr_t NetTx(SharedPkt pkt);
    public:
        
    private:
        NetInfo* netinfo_;              // 有关网卡的信息,比如ip地址、掩码、mac地址等等
        int netif_fd_;                  // 每个网卡驱动的fd

        ConcurrentQueue<SharedPkt> recv_queue_;   // 接收数据包队列
        ConcurrentQueue<SharedPkt> send_queue_;   // 发送数据包队列
        int queue_max_threshold_ = DEFAULT_TX_QUEUE_LEN;              // 队列存储数据包最大个数

        static NetInterface* kLoopNetinterface;
    };
    
    /**
     * @brief 获取回环网卡接口
     * 
     * @return NetInterface* 
     */
    NetInterface* GetLoopNetinterface();

    /**
     * @brief 用来处理从网卡接收来的数据包,对其进行拆分
     * 
     * @param iface 
     */
    void HandleRecvPktCallback(NetInterface* iface);
}