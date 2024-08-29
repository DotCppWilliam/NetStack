#pragma once

#include "net_err.h"
#include "net_interface.h"
#include "sys_plat.h"

namespace netstack 
{
    class NetIfLoop : public NetInterface
    {
    public:
        NetIfLoop(NetInfo* info);
        ~NetIfLoop();

        NetErr_t Open(void* arg) override;
        NetErr_t Close() override;
        NetErr_t Send() override;
    private:
        NetInterface* netif_;
    };
}