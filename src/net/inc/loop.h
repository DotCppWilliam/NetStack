#pragma once

#include "net_interface.h"
#include "sys_plat.h"

namespace netstack 
{
    class NetIfLoop : public NetInterface
    {
    public:
        NetIfLoop(NetInfo* info);
        ~NetIfLoop();
    };

}