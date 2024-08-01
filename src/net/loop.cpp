
#include "loop.h"
#include "net_interface.h"

namespace net 
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

        return NET_ERR_OK;
    }
}