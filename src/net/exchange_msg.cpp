#include "exchange_msg.h"
#include "auto_lock.h"
#include "sys_plat.h"

namespace netstack 
{
    ExchangeMsg::ExchangeMsg()
    {
        work_thread_.SetThreadFunc(&ExchangeMsg::WorkThreadFunc, this, nullptr);
    }

    ExchangeMsg::~ExchangeMsg()
    {

    }
    
    void ExchangeMsg::SendMsg(NetInterface* netif, bool recv_pkt)
    {
        AutoLock lock(lock_);
        msg_deque_.push_back({ netif, recv_pkt });
    }

    void ExchangeMsg::WorkThreadFunc(void* arg)
    {
        // 工作线程, 处理数据包
        // TODO:

    }
}
