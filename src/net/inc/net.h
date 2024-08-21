#pragma once

#include "net_err.h"
#include "net_interface.h"
#include "net_pcap.h"
#include "singlton.h"
#include "exchange_msg.h"
#include "sys_plat.h"

namespace netstack 
{   
    static std::list<NetInterface*> list { nullptr};

    struct NetInfo;
    class NetInit 
    {
    public:
        NetInit()     // TODO: 构造pcap_传入所有网卡信息的list
            : pcap_(list)   // TODO: 临时解决编译报错
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

        ExchangeMsg* GetExchangeMsg() 
        { return &exchange_msg_; }

        NetifPcap* GetNetifPcap()
        { 
            return &pcap_;
        }
        NetInfo* GetNetworkInfo(std::string name = "", std::string ip = "",
                       NetIfType type = NETIF_TYPE_NONE)
        {
            driver_.GetNetworkPtr(name, ip, type);
            return nullptr;
        }
    private:
        NetErr_t Init();
        NetErr_t Start();
        bool initialized_ = false;
        ExchangeMsg exchange_msg_;  // 消息队列
        NetifPcap pcap_;
        PcapNICDriver driver_;
    };

}
