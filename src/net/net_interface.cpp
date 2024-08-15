#include "net_interface.h"
#include "net.h"
#include "packet_buffer.h"
#include "net_err.h"
#include "sys_plat.h"
#include <memory>

namespace netstack 
{   
    std::list<NetInterface*> NetInterface::netif_lists_;
    NetInterface* NetInterface::default_netif_;

    // 默认发送数据包的网卡
    NetInterface* kDefaultNetIf = nullptr;

    NetInterface::NetInterface(void* arg, const char* device_name, int queue_max_threshold)
        : state_(NETIF_CLOSED),
        mtu_(0),
        queue_max_threshold_(0),
        ops_data_(arg),
        name_(device_name), device_(nullptr),
        recv_queue_(queue_max_threshold),
        send_queue_(queue_max_threshold)
    {
        default_netif_ = this;  // 默认指向当前网卡
    }

    NetInterface::~NetInterface()
    {
        
    }

    NetErr_t NetInterface::Open(void* arg)
    {
        

        return NET_ERR_OK;
    }

    NetErr_t NetInterface::Close()
    {
        if (state_ == NETIF_ACTIVE)
        {
            // 日志输出: TODO: 激活状态下不能关闭网卡
            return NET_ERR_STATE;
        }

        // TODO: 关闭操作

        state_ = NETIF_CLOSED;
        netif_lists_.remove(this);
        return NET_ERR_OK;
    }

    NetErr_t NetInterface::Send()
    {

        return NET_ERR_OK;
    }


    void NetInterface::SetAddr(IpAddr& ip, IpAddr& netmask, IpAddr& gateway)
    {
        ipaddr_ = ip;
        netmask_ = netmask;
        gateway_ = gateway;
    }


    void NetInterface::SetMacAddr(std::string& mac_addr)
    {
        mac_addr_ = mac_addr;
    }


    NetErr_t NetInterface::SetActiveState()
    {
        if (state_ != NETIF_OPENED)
        {
            // 日志: 网络没有打开
            return NET_ERR_STATE;
        }
        state_ = NETIF_ACTIVE;
        return NET_ERR_OK;
    }


    NetErr_t NetInterface::SetDeActiveState()
    {
        if (state_ != NETIF_ACTIVE)
        {
            // 日志: TODO: 没有激活
            return NET_ERR_STATE;
        }
        state_ = NETIF_OPENED;
        recv_queue_.Clear();    // TODO: 并发队列的Clear还没有做
        send_queue_.Clear();

        return NET_ERR_OK;
    }


    void NetInterface::DisplayInfo()
    {
        std::string state;
        std::string ip, netmask;

        IpAddr2Str(ipaddr_, ip);
        IpAddr2Str(netmask_, netmask);

        switch (static_cast<int>(state_)) 
        {
            case NETIF_ACTIVE:
                state = "RUNNING";
                break;
            case NETIF_DEACTIVE:
            case NETIF_OPENED:
                state = "OPEND";
                break;
            case NETIF_CLOSED:
                state = "CLOSED";
                break;
        }

        printf("Network card information: \n");
        printf("%s: flags<%s> mtu %llu\n", name_.c_str(), state.c_str(), mtu_);
        
        printf("\t\tinet: %s  netmask:  %s\n", ip.c_str(), netmask.c_str());
        printf("ether: %s  txqueuelen: %d (%s)\n\n", mac_addr_.c_str(), queue_max_threshold_, 
            type_ == NETIF_TYPE_LOOP ? "Local Loopback" : "Ehternet");
    }


    void NetInterface::SetDefaultNetif(NetInterface* netif)
    {
        default_netif_ = netif;
    }



    NetErr_t NetInterface::PushPacket(std::shared_ptr<PacketBuffer> pkt, bool is_recv_queue, bool wait)
    {
        ConcurrentQueue<std::shared_ptr<PacketBuffer>>* queue;
        if (is_recv_queue)
            queue = &recv_queue_;
        else 
            queue = &send_queue_;
        
        if (wait)
        {
            while (!queue->TryEmplace(pkt));
        }
        else 
        {
            bool ret = queue->TryEmplace(pkt);
            if (!ret)
                return NET_ERR_FULL;
        }

        NetInit::GetInstance()->GetExchangeMsg()->SendMsg(this, is_recv_queue);
        return NET_ERR_OK;
    }


    NetErr_t NetInterface::PopPacket(std::shared_ptr<PacketBuffer>, bool is_recv_queue, bool wait)
    {
        std::shared_ptr<PacketBuffer> pkt;
        ConcurrentQueue<std::shared_ptr<PacketBuffer>>* queue;
        if (is_recv_queue)
            queue = &recv_queue_;
        else 
            queue = &send_queue_;
        
        if (wait)
        {
            while (!queue->TryPop(pkt));
        }
        else 
        {
            bool ret = queue->TryPop(pkt);
            if (!ret)
                return NET_ERR_EMPTY;    
        }

        // exmsg 通知消息 TODO:
        return NET_ERR_OK;
    }








    NetErr_t NetInterface::Out(IpAddr& addr, std::shared_ptr<PacketBuffer> pkt)
    {
        NetErr_t ret = PushPacket(pkt, false);
        Send(); // 网卡发送数据
        return ret;
    }
}