#pragma once

#include "net_err.h"

namespace net 
{
    class ExchangeMsg
    {
    public:
    
        NetErr_t Init();
    private:
        static void ThreadFunc(void* arg);
    };
}