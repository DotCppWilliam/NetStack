#include "exchange_msg.h"
#include "net_err.h"
#include "sys_plat.h"

namespace net 
{
    NetErr_t ExchangeMsg::Init()
    {
        lpcap::SysThread thread(&ExchangeMsg::ThreadFunc, nullptr);
        if (!thread.Create())
            return NET_ERR_SYS;

        return NET_ERR_OK;
    }

    

    void ExchangeMsg::ThreadFunc(void* arg)
    {
        while (true)
        {
            plat_sleep(1);
        }
    }
}
