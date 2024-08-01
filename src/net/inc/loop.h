#pragma once

#include "net_err.h"
#include "net_interface.h"

namespace net 
{
    class NetIfLoop : public NetInterface
    {
    public:
        NetIfLoop();
        ~NetIfLoop();

        NetErr_t Open(void* arg) override;
        NetErr_t Close() override;
        NetErr_t Send() override;
    private:
        NetInterface* netif_;
    };
}