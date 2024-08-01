#include "net_interface.h"
#include "PacketBuffer.h"
#include "net_err.h"

namespace net 
{   
    std::list<NetInterface*> NetInterface::netif_lists_;
    NetInterface* NetInterface::default_netif_;

    // 默认发送数据包的网卡
    NetInterface* kDefaultNetIf = nullptr;

    NetInterface::NetInterface(void* arg, const char* device_name, int queue_max_threshold)
        : node_(nullptr), 
        state_(NETIF_CLOSED),
        mtu_(0),
        queue_max_threshold_(0),
        ops_data_(arg),
        name_(device_name),
        recv_queue_(queue_max_threshold),
        send_queue_(queue_max_threshold)
    {

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



    NetErr_t NetInterface::PushPacket(PacketBuffer& pkt, bool is_recv_queue, bool wait)
    {
        

        return NET_ERR_OK;
    }


    PacketBuffer& NetInterface::PopPacket(bool is_recv_queue, bool wait)
    {
        PacketBuffer pkt(100);

        return pkt;
    }
}