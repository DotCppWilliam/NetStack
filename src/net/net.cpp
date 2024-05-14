#include "net.h"


namespace net 
{
    NetErr_t NetInit::Init()
    {
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

        if (exchange_msg_.Init() != NET_ERR_OK)
        {
            // 日志
            throw "NetInit::Initialization(): initialization exchange msg failed";
        }
    }
}
