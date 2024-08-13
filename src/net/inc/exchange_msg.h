#pragma once

#include "net_err.h"

namespace netstack 
{
    class ExchangeMsg
    {
    public:
    
        NetErr_t Init();
    private:
        static void ThreadFunc(void* arg);
    };
}