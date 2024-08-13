#include "net.h"
#include "net_err.h"


namespace netstack 
{
    NetErr_t NetInit::Init()
    {
        if (exchange_msg_.Init() != NET_ERR_OK)
        {
            // 日志
            throw "NetInit::Init(): initialization exchange msg failed";
        }

        if (pcap_.OpenDevice() != NET_ERR_OK)
        {
            // 日志
            throw "NetInit::Init(): initialization Pcap dev failed";
        }
        return NET_ERR_OK;
    }


    NetErr_t NetInit::Start()
    {
        return NET_ERR_OK;
    }

    void NetInit::Initialization()
    {
        if (initialized_) return;
        
        
        if (Init() != NET_ERR_OK)
        {
            // 日志
            throw "NetInit::Initialization(): initialization failed";
        }
        if (Start() != NET_ERR_OK)
        {
            // 日志
            throw "NetInit::Initialization(): start failed";
        }
    }
}
