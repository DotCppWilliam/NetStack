#pragma once

#include "net_err.h"
#include "net_pcap.h"
#include "singlton.h"
#include "exchange_msg.h"
#include "sys_plat.h"

namespace netstack 
{   
    class NetInit 
    {
    public:
        NetInit() 
        {
            
        }
        ~NetInit()
        {

        }
        static NetInit* GetInstance()
        {  return Singleton<NetInit>::get(); }

        void Initialization();

        PcapNICDriver* GetNICDriver()
        { return &driver_; }
    private:
        NetErr_t Init();
        NetErr_t Start();
        bool initialized_ = false;
        ExchangeMsg exchange_msg_;
        NetifPcap pcap_;
        PcapNICDriver driver_;
    };

}
