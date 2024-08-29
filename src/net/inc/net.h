#pragma once

#include "net_err.h"
#include "net_interface.h"
#include "net_pcap.h"
#include "singlton.h"
#include "exchange_msg.h"
#include "sys_plat.h"
#include "timer.h"

namespace netstack 
{   
    static std::list<NetInterface*> list { nullptr};

    struct NetInfo;
    class NetInit 
    {
    public:
        NetInit();
        ~NetInit();
    public:
        static NetInit* GetInstance()
        {  return Singleton<NetInit>::get(); }

        PcapNICDriver* GetNICDriver()
        { return driver_; }

        ExchangeMsg* GetExchangeMsg() 
        { return exchange_msg_; }

        NetifPcap* GetMsgWorker()
        { 
            return msg_worker_;
        }

        NetInfo* GetNetworkInfo(uint32_t ip = 0, std::string name = "",
                       NetIfType type = NETIF_TYPE_NONE)
        {
            driver_->GetNetworkPtr(name, ip, type);
            return nullptr;
        }

        Timer* GetTimer() 
        { return timer_; }
        
        NetErr_t Init();
    private:
        bool initialized_ = false;
        ExchangeMsg* exchange_msg_;  // 消息队列
        NetifPcap* msg_worker_;
        PcapNICDriver* driver_ = nullptr;
        Timer* timer_ = nullptr;
    };
}
