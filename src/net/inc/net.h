#pragma once

#include "net_err.h"
#include "singlton.h"
#include "exchange_msg.h"

namespace net 
{   
    class NetInit 
    {
    public:
        NetInit() {}
        ~NetInit()
        {

        }
        static NetInit* GetInstance()
        {  return util::Singleton<NetInit>::get(); }

        void Initialization();
    private:
        NetErr_t Init();
        NetErr_t Start();
        bool initialized_ = false;
        ExchangeMsg exchange_msg_;
    };

}
