
#include "loop.h"
#include "packet_buffer.h"
#include "net_err.h"
#include "net_interface.h"

namespace netstack 
{

    NetIfLoop::NetIfLoop()
        : NetInterface(nullptr, "loop")
    {
        SetDefaultNetif(this);  // 设置默认网卡为loop(回环)
    }


    NetIfLoop::~NetIfLoop()
    {
        if (netif_)
        {
            delete netif_;
            netif_ = nullptr;
        }
    }


    NetErr_t NetIfLoop::Open(void* arg)
    {

        
        return NET_ERR_OK;
    }

    NetErr_t NetIfLoop::Close()
    {

        return NET_ERR_OK;
    }

    NetErr_t NetIfLoop::Send()
    {
        std::shared_ptr<PacketBuffer> pkt;
        // 从输出队列中取数据
        NetErr_t ret = PopPacket(pkt, false);
        if (ret < 0)
            return ret;

        PushPacket(pkt);    // 向输入队列中压入数据
        // TODO: 通知工作线程当前网卡有数据 exmsg
        return NET_ERR_OK;
    }
}